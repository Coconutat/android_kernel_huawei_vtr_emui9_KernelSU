/*
 * ILITEK Touch IC driver
 *
 * Copyright (C) 2011 ILI Technology Corporation.
 *
 * Author: Dicky Chiang <dicky_chiang@ilitek.com>
 * Based on TDD v7.0 implemented by Mstar & ILITEK
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
 *
 */

#include "ilitek_common.h"
#include "ilitek_dts.h"
#include "ilitek_protocol.h"
#include "ilitek_config.h"
#include "ilitek_report.h"
#include "ilitek_parser.h"
#include "ilitek_test.h"
#include "ilitek_upgrade.h"
#include "ilitek_firmware.h"

#define ILITEK_CMD_MIN                         2
#define ILITEK_CMD_MAX                         512
#define USER_STR_BUFF                          128
#define ILITEK_IOCTL_MAGIC                     100
#define ILITEK_IOCTL_MAXNR                     18

#define ILITEK_IOCTL_I2C_WRITE_DATA            _IOWR(ILITEK_IOCTL_MAGIC, 0, u8*)
#define ILITEK_IOCTL_I2C_SET_WRITE_LENGTH      _IOWR(ILITEK_IOCTL_MAGIC, 1, int)
#define ILITEK_IOCTL_I2C_READ_DATA             _IOWR(ILITEK_IOCTL_MAGIC, 2, u8*)
#define ILITEK_IOCTL_I2C_SET_READ_LENGTH       _IOWR(ILITEK_IOCTL_MAGIC, 3, int)

#define ILITEK_IOCTL_TP_HW_RESET               _IOWR(ILITEK_IOCTL_MAGIC, 4, int)
#define ILITEK_IOCTL_TP_POWER_SWITCH           _IOWR(ILITEK_IOCTL_MAGIC, 5, int)
#define ILITEK_IOCTL_TP_REPORT_SWITCH          _IOWR(ILITEK_IOCTL_MAGIC, 6, int)
#define ILITEK_IOCTL_TP_IRQ_SWITCH             _IOWR(ILITEK_IOCTL_MAGIC, 7, int)

#define ILITEK_IOCTL_TP_DEBUG_LEVEL            _IOWR(ILITEK_IOCTL_MAGIC, 8, int)
#define ILITEK_IOCTL_TP_FUNC_MODE              _IOWR(ILITEK_IOCTL_MAGIC, 9, int)

#define ILITEK_IOCTL_TP_FW_VER                 _IOWR(ILITEK_IOCTL_MAGIC, 10, u8*)
#define ILITEK_IOCTL_TP_PL_VER                 _IOWR(ILITEK_IOCTL_MAGIC, 11, u8*)
#define ILITEK_IOCTL_TP_CORE_VER               _IOWR(ILITEK_IOCTL_MAGIC, 12, u8*)
#define ILITEK_IOCTL_TP_DRV_VER                _IOWR(ILITEK_IOCTL_MAGIC, 13, u8*)
#define ILITEK_IOCTL_TP_CHIP_ID                _IOWR(ILITEK_IOCTL_MAGIC, 14, u32*)

#define ILITEK_IOCTL_TP_NETLINK_CTRL           _IOWR(ILITEK_IOCTL_MAGIC, 15, int*)
#define ILITEK_IOCTL_TP_NETLINK_STATUS         _IOWR(ILITEK_IOCTL_MAGIC, 16, int*)

#define ILITEK_IOCTL_TP_MODE_CTRL              _IOWR(ILITEK_IOCTL_MAGIC, 17, u8*)
#define ILITEK_IOCTL_TP_MODE_STATUS            _IOWR(ILITEK_IOCTL_MAGIC, 18, int*)

u8 g_user_buf[USER_STR_BUFF] = { 0 };

static ssize_t ilitek_proc_debug_switch_read(struct file *pFile, char __user *buff, size_t nCount, loff_t *pPos)
{
    int res = 0;

    if (*pPos != 0)
        return 0;

    memset(g_user_buf, 0, USER_STR_BUFF * sizeof(unsigned char));

    g_ilitek_ts->debug_node_open = !g_ilitek_ts->debug_node_open;

    ilitek_info(" %s debug_flag message = %x", g_ilitek_ts->debug_node_open ? "Enabled" : "Disabled", g_ilitek_ts->debug_node_open);

    nCount = sprintf(g_user_buf, "g_ilitek_ts->debug_node_open : %s", g_ilitek_ts->debug_node_open ? "Enabled" : "Disabled");

    *pPos += nCount;

    res = copy_to_user(buff, g_user_buf, nCount);
    if (res < 0) {
        ilitek_err("Failed to copy data to user space");
    }

    return nCount;
}

static ssize_t ilitek_proc_debug_message_write(struct file *filp, const char *buff, size_t size, loff_t *pPos)
{
    int ret = 0;
    u8 cmd[ILITEK_CMD_MAX] = { 0 };

    if (size < ILITEK_CMD_MIN ||
        size >= ILITEK_CMD_MAX ||
        *pPos != 0 ||
        IS_ERR_OR_NULL(buff)) {
        ilitek_err("write size or buff is invaild\n");
        return -EINVAL;
    }

    ret = copy_from_user(cmd, buff, size - 1);
    if (ret) {
        ilitek_err("copy data from user space, failed");
        return -EINVAL;
    }

    if (strcmp(cmd, "dbg_flag") == 0) {
        g_ilitek_ts->debug_node_open = !g_ilitek_ts->debug_node_open;
        ilitek_info(" %s debug_flag message(%X).\n", g_ilitek_ts->debug_node_open ? "Enabled" : "Disabled",
             g_ilitek_ts->debug_node_open);
    }
    return size;
}

static ssize_t ilitek_proc_debug_message_read(struct file *filp, char __user *buff, size_t size, loff_t *pPos)
{
    unsigned long p = *pPos;
    unsigned int count = size;
    int i = 0;
    int send_data_len = 0;
    size_t ret = 0;
    int data_count = 0;
    int one_data_bytes = 0;
    int need_read_data_len = 0;
    int type = 0;
    unsigned char *tmpbuf = NULL;
    unsigned char tmpbufback[128] = { 0 };

    mutex_lock(&g_ilitek_ts->ilitek_debug_read_mutex);

    while (g_ilitek_ts->debug_data_frame <= 0) {
        if (filp->f_flags & O_NONBLOCK) {
            return -EAGAIN;
        }
        wait_event_interruptible(g_ilitek_ts->inq, g_ilitek_ts->debug_data_frame > 0);
    }

    mutex_lock(&g_ilitek_ts->ilitek_debug_mutex);

    tmpbuf = vmalloc(4096);    /* buf size if even */
    if (ERR_ALLOC_MEM(tmpbuf)) {
        ilitek_err("buffer vmalloc error\n");
        send_data_len += sprintf(tmpbufback + send_data_len, "buffer vmalloc error\n");
        ret = copy_to_user(buff, tmpbufback, send_data_len); /*g_ilitek_ts->debug_buf[0] */
    } else {
        if (g_ilitek_ts->debug_data_frame > 0) {
            if (g_ilitek_ts->debug_buf[0][0] == 0x5A) {
                need_read_data_len = 43;
            } else if (g_ilitek_ts->debug_buf[0][0] == 0x7A) {
                type = g_ilitek_ts->debug_buf[0][3] & 0x0F;

                data_count = g_ilitek_ts->debug_buf[0][1] * g_ilitek_ts->debug_buf[0][2];

                if (type == 0 || type == 1 || type == 6) {
                    one_data_bytes = 1;
                } else if (type == 2 || type == 3) {
                    one_data_bytes = 2;
                } else if (type == 4 || type == 5) {
                    one_data_bytes = 4;
                }
                need_read_data_len = data_count * one_data_bytes + 1 + 5;
            }

            send_data_len = 0;    /* g_ilitek_ts->debug_buf[0][1] - 2; */
            need_read_data_len = 2040;
            if (need_read_data_len <= 0) {
                ilitek_err("parse data err data len = %d\n", need_read_data_len);
                send_data_len +=
                    sprintf(tmpbuf + send_data_len, "parse data err data len = %d\n",
                        need_read_data_len);
            } else {
                for (i = 0; i < need_read_data_len; i++) {
                    send_data_len += sprintf(tmpbuf + send_data_len, "%02X", g_ilitek_ts->debug_buf[0][i]);
                    if (send_data_len >= 4096) {
                        ilitek_err("send_data_len = %d set 4096 i = %d\n", send_data_len, i);
                        send_data_len = 4096;
                        break;
                    }
                }
            }
            send_data_len += sprintf(tmpbuf + send_data_len, "\n\n");

            if (p == 5 || size == 4096 || size == 2048) {
                g_ilitek_ts->debug_data_frame--;
                if (g_ilitek_ts->debug_data_frame < 0) {
                    g_ilitek_ts->debug_data_frame = 0;
                }

                for (i = 1; i <= g_ilitek_ts->debug_data_frame; i++) {
                    memcpy(g_ilitek_ts->debug_buf[i - 1], g_ilitek_ts->debug_buf[i], 2048);
                }
            }
        } else {
            ilitek_err("no data send\n");
            send_data_len += sprintf(tmpbuf + send_data_len, "no data send\n");
        }

        /* Preparing to send data to user */
        if (size == 4096)
            ret = copy_to_user(buff, tmpbuf, send_data_len);
        else
            ret = copy_to_user(buff, tmpbuf + p, send_data_len - p);

        if (ret) {
            ilitek_err("copy_to_user err\n");
            ret = -EFAULT;
        } else {
            *pPos += count;
            ret = count;
            ilitek_debug(DEBUG_FINGER_REPORT, "Read %d bytes(s) from %ld\n", count, p);
        }
    }
    /* ilitek_err("send_data_len = %d\n", send_data_len); */
    if (send_data_len <= 0 || send_data_len > 4096) {
        ilitek_err("send_data_len = %d set 2048\n", send_data_len);
        send_data_len = 4096;
    }
    if (tmpbuf != NULL) {
        vfree(tmpbuf);
        tmpbuf = NULL;
    }

    mutex_unlock(&g_ilitek_ts->ilitek_debug_mutex);
    mutex_unlock(&g_ilitek_ts->ilitek_debug_read_mutex);
    return send_data_len;
}

static ssize_t ilitek_proc_mp_test_read(struct file *filp, char __user *buff, size_t size, loff_t *pPos)
{
    u32 len = 0;

    if (*pPos != 0)
        return 0;

    if (core_parser_path(INI_NAME_PATH) < 0) {
        ilitek_err("Failed to parsing INI file\n");
        goto out;
    }

    if (ilitek_test_init()) {
        ilitek_err("alloc test data failed\n");
        goto out;
    }

    /* Init MP structure */
    if(core_mp_init() < 0) {
        ilitek_err("Failed to init mp\n");
        goto out;
    }

    ilitek_config_disable_report_irq();

    /* Switch to Test mode */
    ilitek_config_mode_ctrl(ILITEK_TEST_MODE, NULL);

    /* Do not chang the sequence of test */
    core_mp_run_test("Noise Peak To Peak(With Panel)", true);
    core_mp_run_test("Noise Peak to Peak(IC Only)", true);
    core_mp_run_test("Short Test -ILI9881", true);
    core_mp_run_test("Open Test(integration)_SP", true);
    core_mp_run_test("Raw Data(Have BK)", true);
    //core_mp_run_test("Raw Data(Have BK) (LCM OFF)", true);
    core_mp_run_test("Calibration Data(DAC)", true);
    core_mp_run_test("Raw Data(No BK)", true);
    core_mp_run_test("Raw Data(No BK) (LCM OFF)", true);
    core_mp_run_test("Noise Peak to Peak(With Panel) (LCM OFF)", true);
    //core_mp_run_test("Noise Peak to Peak(IC Only) (LCM OFF)", true);
    // core_mp_run_test("Raw Data_TD(LCM OFF)", true);
    // core_mp_run_test("Peak To Peak_TD(LCM OFF)", true);
    // core_mp_run_test("Doze Raw Data", true);
    // core_mp_run_test("Doze Peak To Peak", true);
    //core_mp_run_test("Pin Test(INT & RST)", true);

    core_mp_show_result();

    core_mp_test_free();

    /* hw reset avoid i2c error */
    ilitek_chip_reset();

    /* Switch to Demo mode */
    ilitek_config_mode_ctrl(ILITEK_DEMO_MODE, NULL);

    ilitek_config_enable_report_irq();

out:
    *pPos = len;
    ilitek_test_exit();
    ilitek_info("MP Test DONE\n");
    return len;
}

static ssize_t ilitek_proc_mp_test_write(struct file *filp, const char *buff, size_t size, loff_t *pPos)
{
    int i, res = 0, count = 0;
    char str[512] = {0};
    char *token = NULL, *cur = NULL;
    u8 *va = NULL;
    u8 cmd[ILITEK_CMD_MAX] = { 0 };

    if (size < ILITEK_CMD_MIN ||
        size >= ILITEK_CMD_MAX ||
        *pPos != 0 ||
        IS_ERR_OR_NULL(buff)) {
        ilitek_err("write size or buff is invaild\n");
        return -EINVAL;
    }

    res = copy_from_user(cmd, buff, size - 1);
    if (res) {
        ilitek_info("copy data from user space, failed\n");
        return -EINVAL;
    }

    ilitek_info("size = %d, cmd = %s\n", (int)size, cmd);

    if (size > 64) {
        ilitek_err("The size of string is too long\n");
        return size;
    }

    token = cur = cmd;

    va = kcalloc(64, sizeof(u8), GFP_KERNEL);

    while ((token = strsep(&cur, ",")) != NULL) {
        va[count] = katoi(token);
        ilitek_info("data[%d] = %x\n", count, va[count]);
        count++;
    }

    ilitek_info("cmd = %s\n", cmd);

    if (ilitek_test_init()) {
        ilitek_err("alloc test data failed\n");
        goto out;
    }

    /* Init MP structure */
    if(core_mp_init() < 0) {
        ilitek_err("Failed to init mp\n");
        goto out;
    }

    for (i = 0; i < core_mp->mp_items; i++) {
        if (strcmp(cmd, tItems[i].name) == 0) {
            strcpy(str, tItems[i].desp);
            tItems[i].run = 1;
            tItems[i].max = va[1];
            tItems[i].min = va[2];
            tItems[i].frame_count = va[3];
            break;
        }
    }

    if (i == core_mp->mp_items) {
        ilitek_err("input invaild, can't find test item\n");
        core_mp_test_free();
        goto out;
    }

    ilitek_config_disable_report_irq();

    /* Switch to Test mode */
    ilitek_config_mode_ctrl(ILITEK_TEST_MODE, NULL);

    core_mp_run_test(str, false);

    core_mp_show_result();

    core_mp_test_free();

    /* hw reset avoid i2c error */
    ilitek_chip_reset();

    /* Switch to Demo mode it prevents if fw fails to be switched */
    ilitek_config_mode_ctrl(ILITEK_DEMO_MODE, NULL);

    ilitek_config_enable_report_irq();

out:
    ilitek_test_exit();
    ipio_kfree((void **)&va);
    ilitek_info("MP Test DONE\n");
    return size;
}

static ssize_t ilitek_proc_debug_level_read(struct file *filp, char __user *buff, size_t size, loff_t *pPos)
{
    int res = 0;
    u32 len = 0;

    if (*pPos != 0)
        return 0;

    memset(g_user_buf, 0, USER_STR_BUFF * sizeof(unsigned char));

    len = sprintf(g_user_buf, "%d", ilitek_debug_level);

    ilitek_info("Current DEBUG Level = %d\n", ilitek_debug_level);
    ilitek_info("You can set one of levels for debug as below:\n");
    ilitek_info("DEBUG_NONE = %d\n", DEBUG_NONE);
    ilitek_info("DEBUG_IRQ = %d\n", DEBUG_IRQ);
    ilitek_info("DEBUG_FINGER_REPORT = %d\n", DEBUG_FINGER_REPORT);
    ilitek_info("DEBUG_FIRMWARE = %d\n", DEBUG_FIRMWARE);
    ilitek_info("DEBUG_CONFIG = %d\n", DEBUG_CONFIG);
    ilitek_info("DEBUG_I2C = %d\n", DEBUG_I2C);
    ilitek_info("DEBUG_BATTERY = %d\n", DEBUG_BATTERY);
    ilitek_info("DEBUG_MP_TEST = %d\n", DEBUG_MP_TEST);
    ilitek_info("DEBUG_IOCTL = %d\n", DEBUG_IOCTL);
    ilitek_info("DEBUG_NETLINK = %d\n", DEBUG_NETLINK);
    ilitek_info("DEBUG_PARSER = %d\n", DEBUG_PARSER);
    ilitek_info("DEBUG_GESTURE = %d\n", DEBUG_GESTURE);
    ilitek_info("DEBUG_ROI = %d\n", DEBUG_ROI);
    ilitek_info("DEBUG_ALL = %d\n", DEBUG_ALL);
    ilitek_info("DRIVER VERSION = %s\n", DRIVER_VERSION);
    ilitek_info("CSV PATH = %s\n", CSV_PATH);
    ilitek_info("INI NAME PATH = %s\n", INI_NAME_PATH);
    ilitek_info("UPDATE FW PATH = %s\n", UPDATE_FW_PATH);
    ilitek_info("FW VERSION = %s\n", g_ilitek_ts->cfg->fw_ver.str);

    res = copy_to_user((u32 *) buff, &ilitek_debug_level, len);
    if (res < 0) {
        ilitek_err("Failed to copy data to user space\n");
    }

    *pPos = len;

    return len;
}

static ssize_t ilitek_proc_debug_level_write(struct file *filp, const char *buff, size_t size, loff_t *pPos)
{
    int res = 0;
    u8 cmd[ILITEK_CMD_MAX] = { 0 };

    if (size < ILITEK_CMD_MIN ||
        size >= ILITEK_CMD_MAX ||
        *pPos != 0 ||
        IS_ERR_OR_NULL(buff)) {
        ilitek_err("write size or buff is invaild\n");
        return -EINVAL;
    }

    res = copy_from_user(cmd, buff, size - 1);
    if (res) {
        ilitek_info("copy data from user space, failed\n");
        return -EINVAL;
    }

    ilitek_debug_level = katoi(cmd);

    ilitek_info("ilitek_debug_level = %d\n", ilitek_debug_level);

    return size;
}

static ssize_t ilitek_proc_gesture_read(struct file *filp, char __user *buff, size_t size, loff_t *pPos)
{
    int res = 0;
    u32 len = 0;

    if (*pPos != 0)
        return 0;

    memset(g_user_buf, 0, USER_STR_BUFF * sizeof(unsigned char));

    len = sprintf(g_user_buf, "%d", g_ilitek_ts->support_gesture);

    ilitek_info("support_gestures = %d\n", g_ilitek_ts->support_gesture);

    res = copy_to_user((u32 *) buff, &g_ilitek_ts->support_gesture, len);
    if (res < 0) {
        ilitek_err("Failed to copy data to user space\n");
    }

    *pPos = len;

    return len;
}

static ssize_t ilitek_proc_gesture_write(struct file *filp, const char *buff, size_t size, loff_t *pPos)
{
    int res = 0;
    u8 cmd[ILITEK_CMD_MAX] = { 0 };

    if (size < ILITEK_CMD_MIN ||
        size >= ILITEK_CMD_MAX ||
        *pPos != 0 ||
        IS_ERR_OR_NULL(buff)) {
        ilitek_err("write size or buff is invaild\n");
        return -EINVAL;
    }

    res = copy_from_user(cmd, buff, size - 1);
    if (res) {
        ilitek_info("copy data from user space, failed\n");
        return -EINVAL;
    }

    ilitek_info("size = %d, cmd = %s\n", (int)size, cmd);

    if (strcmp(cmd, "on") == 0) {
        ilitek_info("enable gesture mode\n");
        g_ilitek_ts->support_gesture = true;
    } else if (strcmp(cmd, "off") == 0) {
        ilitek_info("disable gesture mode\n");
        g_ilitek_ts->support_gesture = false;
    } else
        ilitek_err("Unknown command\n");

    return size;
}
#ifdef BATTERY_CHECK
static ssize_t ilitek_proc_check_battery_read(struct file *filp, char __user *buff, size_t size, loff_t *pPos)
{
    int res = 0;
    u32 len = 0;

    if (*pPos != 0)
        return 0;

    memset(g_user_buf, 0, USER_STR_BUFF * sizeof(unsigned char));

    len = sprintf(g_user_buf, "%d", g_ilitek_ts->isEnablePollCheckPower);

    ilitek_info("isEnablePollCheckPower = %d\n", g_ilitek_ts->isEnablePollCheckPower);

    res = copy_to_user((u32 *) buff, &g_ilitek_ts->isEnablePollCheckPower, len);
    if (res < 0) {
        ilitek_err("Failed to copy data to user space\n");
    }

    *pPos = len;

    return len;
}

static ssize_t ilitek_proc_check_battery_write(struct file *filp, const char *buff, size_t size, loff_t *pPos)
{
    int res = 0;
    u8 cmd[ILITEK_CMD_MAX] = { 0 };

    if (size < ILITEK_CMD_MIN ||
        size >= ILITEK_CMD_MAX ||
        *pPos != 0 ||
        IS_ERR_OR_NULL(buff)) {
        ilitek_err("write size or buff is invaild\n");
        return -EINVAL;
    }

    res = copy_from_user(cmd, buff, size - 1);
    if (res) {
        ilitek_info("copy data from user space, failed\n");
        return -EINVAL;
    }

    ilitek_info("size = %d, cmd = %s\n", (int)size, cmd);

#ifdef ENABLE_BATTERY_CHECK
    if (strcmp(cmd, "on") == 0) {
        ilitek_info("Start the thread of check power status\n");
        queue_delayed_work(g_ilitek_ts->check_power_status_queue, &g_ilitek_ts->check_power_status_work, g_ilitek_ts->work_delay);
        g_ilitek_ts->isEnablePollCheckPower = true;
    } else if (strcmp(cmd, "off") == 0) {
        ilitek_info("Cancel the thread of check power status\n");
        cancel_delayed_work_sync(&g_ilitek_ts->check_power_status_work);
        g_ilitek_ts->isEnablePollCheckPower = false;
    } else
        ilitek_err("Unknown command\n");
#else
    ilitek_err("You need to enable its MACRO before operate it.\n");
#endif

    return size;
}
#endif
static ssize_t ilitek_proc_fw_process_read(struct file *filp, char __user *buff, size_t size, loff_t *pPos)
{
    int res = 0;
    u32 len = 0;

    /*
     * If file position is non-zero,  we assume the string has been read
     * and indicates that there is no more data to be read.
     */
    if (*pPos != 0)
        return 0;

    memset(g_user_buf, 0, USER_STR_BUFF * sizeof(unsigned char));

    len = sprintf(g_user_buf, "%02d", core_firmware->update_status);

    ilitek_info("update status = %d\n", core_firmware->update_status);

    res = copy_to_user((u32 *) buff, &core_firmware->update_status, len);
    if (res < 0) {
        ilitek_err("Failed to copy data to user space");
    }

    *pPos = len;

    return len;
}

/*
 * To avoid the restriction of selinux, we assigned a fixed path where locates firmware file,
 * reading (cat) this node to notify driver running the upgrade process from user space.
 */
static ssize_t ilitek_proc_fw_upgrade_read(struct file *filp, char __user *buff, size_t size, loff_t *pPos)
{
    int res = 0;
    u32 len = 0;

    ilitek_info("Preparing to upgarde firmware\n");

    if (*pPos != 0)
        return 0;

    ilitek_config_disable_report_irq();
#ifdef BATTERY_CHECK
    if (g_ilitek_ts->isEnablePollCheckPower)
        cancel_delayed_work_sync(&g_ilitek_ts->check_power_status_work);
#endif
    res = core_firmware_upgrade(UPDATE_FW_PATH, false);

    ilitek_config_enable_report_irq();

#ifdef BATTERY_CHECK
    if (g_ilitek_ts->isEnablePollCheckPower)
        queue_delayed_work(g_ilitek_ts->check_power_status_queue, &g_ilitek_ts->check_power_status_work, g_ilitek_ts->work_delay);
#endif

    if (res < 0) {
        core_firmware->update_status = res;
        ilitek_err("Failed to upgrade firwmare\n");
    } else {
        core_firmware->update_status = 100;
        ilitek_info("Succeed to upgrade firmware\n");
    }

    *pPos = len;

    return len;
}

static ssize_t ilitek_proc_iram_upgrade_read(struct file *filp, char __user *buff, size_t size, loff_t *pPos)
{
    int res = 0;
    u32 len = 0;

    ilitek_info("Preparing to upgarde firmware by IRAM\n");

    if (*pPos != 0)
        return 0;

    ilitek_config_disable_report_irq();

    res = core_firmware_upgrade(UPDATE_FW_PATH, true);

    ilitek_config_enable_report_irq();

    if (res < 0) {
        /* return the status to user space even if any error occurs. */
        core_firmware->update_status = res;
        ilitek_err("Failed to upgrade firwmare by IRAM, res = %d\n", res);
    } else {
        ilitek_info("Succeed to upgrade firmware by IRAM\n");
    }

    *pPos = len;

    return len;
}

/* for debug */
static ssize_t ilitek_proc_ioctl_read(struct file *filp, char __user *buff, size_t size, loff_t *pPos)
{
    int res = 0;
    u32 len = 0;
    u8 cmd[ILITEK_CMD_MAX] = { 0 };

    if (size < ILITEK_CMD_MIN ||
        size >= ILITEK_CMD_MAX ||
        *pPos != 0 ||
        IS_ERR_OR_NULL(buff)) {
        ilitek_err("write size or buff is invaild\n");
        return -EINVAL;
    }

    res = copy_from_user(cmd, buff, size - 1);
    if (res) {
        ilitek_info("copy data from user space, failed\n");
        return -EINVAL;
    }

    ilitek_info("size = %d, cmd = %d", (int)size, cmd[0]);

    /* test */
    if (cmd[0] == 0x1) {
        ilitek_info("HW Reset\n");
        ilitek_chip_reset();
    } else if (cmd[0] == 0x02) {
        ilitek_info("Disable report irq data\n");
        ilitek_config_disable_report_irq();
    } else if (cmd[0] == 0x03) {
        ilitek_info("Enable report irq data\n");
        ilitek_config_enable_report_irq();
    } else if (cmd[0] == 0x04) {
        ilitek_info("Get Chip id\n");
        ilitek_config_get_chip_id(0);
    }

    *pPos = len;

    return len;
}

/* for debug */
static ssize_t ilitek_proc_ioctl_write(struct file *filp, const char *buff, size_t size, loff_t *pPos)
{
    int res = 0, count = 0, i;
    int w_len = 0, r_len = 0, i2c_delay = 0;
    char *token = NULL, *cur = NULL;
    u8 temp[256] = { 0 };
    u8 *data = NULL;
    u8 cmd[ILITEK_CMD_MAX] = { 0 };
    int reset_gpio = g_ilitek_ts->ts_dev_data->ts_platform_data->reset_gpio;

    if (size < ILITEK_CMD_MIN ||
        size >= ILITEK_CMD_MAX ||
        *pPos != 0 ||
        IS_ERR_OR_NULL(buff)) {
        ilitek_err("write size or buff is invaild\n");
        return -EINVAL;
    }

    res = copy_from_user(cmd, buff, size - 1);
    if (res) {
        ilitek_info("copy data from user space, failed\n");
        return -EINVAL;
    }

    ilitek_info("size = %d, cmd = %s\n", (int)size, cmd);

    token = cur = cmd;

    data = kmalloc(512 * sizeof(u8), GFP_KERNEL);
    memset(data, 0, 512);

    while ((token = strsep(&cur, ",")) != NULL) {
        data[count] = str2hex(token);
        ilitek_info("data[%d] = %x\n",count, data[count]);
        count++;
    }

    ilitek_info("cmd = %s\n", cmd);

    if (strcmp(cmd, "reset") == 0) {
        ilitek_info("HW Reset\n");
        ilitek_chip_reset();
    } else if (strcmp(cmd, "disirq") == 0) {
        ilitek_info("Disable report irq data\n");
        ilitek_config_disable_report_irq();
    } else if (strcmp(cmd, "enairq") == 0) {
        ilitek_info("Enable report irq data\n");
        ilitek_config_enable_report_irq();
    } else if (strcmp(cmd, "getchip") == 0) {
        ilitek_info("Get Chip id\n");
        ilitek_config_get_chip_id(0);
    } else if (strcmp(cmd, "dispcc") == 0) {
        ilitek_info("disable phone cover\n");
        ilitek_config_phone_cover_ctrl(CMD_DISABLE);
    } else if (strcmp(cmd, "enapcc") == 0) {
        ilitek_info("enable phone cover\n");
        ilitek_config_phone_cover_ctrl(CMD_ENABLE);
    } else if (strcmp(cmd, "disfsc") == 0) {
        ilitek_info("disable finger sense\n");
        ilitek_config_finger_sense_ctrl(CMD_DISABLE);
    } else if (strcmp(cmd, "enafsc") == 0) {
        ilitek_info("enable finger sense\n");
        ilitek_config_finger_sense_ctrl(CMD_ENABLE);
    } else if (strcmp(cmd, "disprox") == 0) {
        ilitek_info("disable proximity\n");
        ilitek_config_proximity_ctrl(CMD_DISABLE);
    } else if (strcmp(cmd, "enaprox") == 0) {
        ilitek_info("enable proximity\n");
        ilitek_config_proximity_ctrl(CMD_ENABLE);
    } else if (strcmp(cmd, "disglove") == 0) {
        ilitek_info("disable glove function\n");
        ilitek_config_glove_ctrl(CMD_DISABLE);
    } else if (strcmp(cmd, "enaglove") == 0) {
        ilitek_info("enable glove function\n");
        ilitek_config_glove_ctrl(CMD_ENABLE);
    } else if (strcmp(cmd, "glovesl") == 0) {
        ilitek_info("set glove as seamless\n");
        ilitek_config_glove_ctrl(CMD_SEAMLESS);
    } else if (strcmp(cmd, "enastylus") == 0) {
        ilitek_info("enable stylus\n");
        ilitek_config_stylus_ctrl(CMD_ENABLE);
    } else if (strcmp(cmd, "disstylus") == 0) {
        ilitek_info("disable stylus\n");
        ilitek_config_stylus_ctrl(CMD_DISABLE);
    } else if (strcmp(cmd, "stylussl") == 0) {
        ilitek_info("set stylus as seamless\n");
        ilitek_config_stylus_ctrl(CMD_SEAMLESS);
    } else if (strcmp(cmd, "tpscan_ab") == 0) {
        ilitek_info("set TP scan as mode AB\n");
        ilitek_config_tp_scan_mode_ctrl(CMD_ENABLE);
    } else if (strcmp(cmd, "tpscan_b") == 0) {
        ilitek_info("set TP scan as mode B\n");
        ilitek_config_tp_scan_mode_ctrl(CMD_DISABLE);
    } else if (strcmp(cmd, "phone_cover") == 0) {
        ilitek_info("set size of phone conver window\n");
        ilitek_config_set_phone_cover(data);
    } else if (strcmp(cmd, "debugmode") == 0) {
        ilitek_info("debug mode test enter\n");
        ilitek_config_disable_report_irq();
        ilitek_config_mode_ctrl(ILITEK_DEBUG_MODE, NULL);
        ilitek_config_enable_report_irq();
    } else if (strcmp(cmd, "baseline") == 0) {
        ilitek_info("test baseline raw\n");
        ilitek_config_disable_report_irq();
        ilitek_config_mode_ctrl(ILITEK_DEBUG_MODE, NULL);
        temp[0] = 0xFA;
        temp[1] = 0x08;
        ilitek_i2c_write(temp, 2);
        ilitek_config_enable_report_irq();
    } else if (strcmp(cmd, "delac_on") == 0) {
        ilitek_info("test get delac\n");
        ilitek_config_disable_report_irq();
        ilitek_config_mode_ctrl(ILITEK_DEBUG_MODE, NULL);
        temp[0] = 0xFA;
        temp[1] = 0x03;
        ilitek_i2c_write(temp, 2);
        ilitek_config_enable_report_irq();
    } else if (strcmp(cmd, "delac_off") == 0) {
        ilitek_info("test get delac\n");
        ilitek_config_disable_report_irq();
        ilitek_config_mode_ctrl(ILITEK_DEMO_MODE, NULL);
        ilitek_config_enable_report_irq();
    }else if (strcmp(cmd, "test") == 0) {
        ilitek_info("test test_reset test 1\n");
        gpio_direction_output(reset_gpio, 1);
        mdelay(1);
        gpio_set_value(reset_gpio, 0);
        mdelay(1);
        gpio_set_value(reset_gpio, 1);
        mdelay(10);
    } else if (strcmp(cmd, "gt") == 0) {
        ilitek_info("test Gesture test\n");
        ilitek_config_load_gesture_code();
    } else if (strcmp(cmd, "gt1") == 0) {
        ilitek_info("test Gesture test 1\n");
        temp[0] = 0x01;
        temp[1] = 0x01;
        temp[2] = 0x00;
        w_len = 3;
        ilitek_i2c_write(temp, w_len);
        if (ilitek_config_check_cdc_busy(50, 100) < 0)
            ilitek_err("Check busy is timout !\n");
    } else if (strcmp(cmd, "gt2") == 0) {
        temp[0] = 0x01;
        temp[1] = 0x0A;
        temp[2] = 0x01;
        w_len = 3;
        ilitek_i2c_write(temp, w_len);
        ilitek_info("test Gesture test\n");
    } else if (strcmp(cmd, "i2c_w") == 0) {
        w_len = data[1];
        ilitek_info("w_len = %d\n", w_len);

        for (i = 0; i < w_len; i++) {
            temp[i] = data[2 + i];
            ilitek_info("i2c[%d] = %x\n", i, temp[i]);
        }

        ilitek_i2c_write(temp, w_len);
    } else if (strcmp(cmd, "i2c_r") == 0) {
        r_len = data[1];
        ilitek_info("r_len = %d\n", r_len);

        ilitek_i2c_read(&temp[0], r_len);

        for (i = 0; i < r_len; i++)
            ilitek_info("i2c[%d] = %x\n", i, temp[i]);
    } else if (strcmp(cmd, "i2c_w_r") == 0) {
        w_len = data[1];
        r_len = data[2];
        i2c_delay = data[3];
        ilitek_info("w_len = %d, r_len = %d, delay = %d\n", w_len, r_len, i2c_delay);

        for (i = 0; i < w_len; i++) {
            temp[i] = data[4 + i];
            ilitek_info("i2c[%d] = %x\n", i, temp[i]);
        }

        ilitek_i2c_write(temp, w_len);

        memset(temp, 0, sizeof(temp));
        mdelay(i2c_delay);

        ilitek_i2c_read(&temp[0], r_len);

        for (i = 0; i < r_len; i++)
            ilitek_info("i2c[%d] = %x\n", i, temp[i]);
    } else if (strcmp(cmd, "dissns") == 0) {
        ilitek_info("sens stop\n");
        ilitek_config_sense_ctrl(CMD_DISABLE);
    } else if (strcmp(cmd, "enasns") == 0) {
        ilitek_info("sens start\n");
        ilitek_config_sense_ctrl(CMD_ENABLE);
    } else if (strcmp(cmd, "disslp") == 0) {
        ilitek_info("sleep in\n");
        ilitek_config_sleep_ctrl(CMD_DISABLE);
    } else if (strcmp(cmd, "enaslp") == 0) {
        ilitek_info("sleep out\n");
        ilitek_config_sleep_ctrl(CMD_ENABLE);
    } else if (strcmp(cmd, "load_ini_everytime") == 0) {
       ilitek_info("load ini file everytime\n");
       g_ilitek_ts->open_threshold_status = false;
       g_ilitek_ts->only_open_once_captest_threshold = false;
    } else if (strcmp(cmd, "load_ini_once") == 0) {
       ilitek_info("load ini file once\n");
       g_ilitek_ts->open_threshold_status = false;
       g_ilitek_ts->only_open_once_captest_threshold = true;
    } else {
        ilitek_err("Unknown command\n");
    }

    ipio_kfree((void **)&data);
    return size;
}

static long ilitek_proc_ioctl(struct file *filp, unsigned int cmd, unsigned long arg)
{
    int res = 0, length = 0;
    u8 szBuf[512] = { 0 };
    static uint16_t i2c_rw_length = 0;
    u32 id_to_user = 0x0;
    char dbg[10] = { 0 };
    struct ilitek_config *p_cfg = g_ilitek_ts->cfg;
    struct ilitek_protocol *p_pro = g_ilitek_ts->pro;
    struct ilitek_report *p_rpt = g_ilitek_ts->rpt;

    ilitek_debug(DEBUG_IOCTL, "cmd = %d\n", _IOC_NR(cmd));

    if (_IOC_TYPE(cmd) != ILITEK_IOCTL_MAGIC) {
        ilitek_err("The Magic number doesn't match\n");
        return -ENOTTY;
    }

    if (_IOC_NR(cmd) > ILITEK_IOCTL_MAXNR) {
        ilitek_err("The number of ioctl doesn't match\n");
        return -ENOTTY;
    }

    switch (cmd) {
    case ILITEK_IOCTL_I2C_WRITE_DATA:
        res = copy_from_user(szBuf, (u8 *) arg, i2c_rw_length);
        if (res < 0) {
            ilitek_err("Failed to copy data from user space\n");
        } else {
            res = ilitek_i2c_write(&szBuf[0], i2c_rw_length);
            if (res < 0) {
                ilitek_err("Failed to write data via i2c\n");
            }
        }
        break;

    case ILITEK_IOCTL_I2C_READ_DATA:
        res = ilitek_i2c_read(szBuf, i2c_rw_length);
        if (res < 0) {
            ilitek_err("Failed to read data via i2c\n");
        } else {
            res = copy_to_user((u8 *) arg, szBuf, i2c_rw_length);
            if (res < 0) {
                ilitek_err("Failed to copy data to user space\n");
            }
        }
        break;

    case ILITEK_IOCTL_I2C_SET_WRITE_LENGTH:
    case ILITEK_IOCTL_I2C_SET_READ_LENGTH:
        i2c_rw_length = arg;
        break;

    case ILITEK_IOCTL_TP_HW_RESET:
        ilitek_chip_reset();
        break;

    case ILITEK_IOCTL_TP_POWER_SWITCH:
        ilitek_info("Not implemented yet\n");
        break;

    case ILITEK_IOCTL_TP_REPORT_SWITCH:
        res = copy_from_user(szBuf, (u8 *) arg, 1);
        if (res < 0) {
            ilitek_err("Failed to copy data from user space\n");
        } else {
            if (szBuf[0]) {
                g_ilitek_ts->isEnableFR = true;
                ilitek_debug(DEBUG_IOCTL, "Function of finger report was enabled\n");
            } else {
                g_ilitek_ts->isEnableFR = false;
                ilitek_debug(DEBUG_IOCTL, "Function of finger report was disabled\n");
            }
        }
        break;

    case ILITEK_IOCTL_TP_IRQ_SWITCH:
        res = copy_from_user(szBuf, (u8 *) arg, 1);
        if (res < 0) {
            ilitek_err("Failed to copy data from user space\n");
        } else {
            if (szBuf[0]) {
                ilitek_config_enable_report_irq();
            } else {
                ilitek_config_disable_report_irq();
            }
        }
        break;

    case ILITEK_IOCTL_TP_DEBUG_LEVEL:
        res = copy_from_user(dbg, (u32 *) arg, sizeof(u32));
        if (res < 0) {
            ilitek_err("Failed to copy data from user space\n");
        } else {
            ilitek_debug_level = katoi(dbg);
            ilitek_info("ipio_debug_level = %d", ilitek_debug_level);
        }
        break;

    case ILITEK_IOCTL_TP_FUNC_MODE:
        res = copy_from_user(szBuf, (u8 *) arg, 3);
        if (res < 0) {
            ilitek_err("Failed to copy data from user space\n");
        } else {
            ilitek_i2c_write(&szBuf[0], 3);
        }
        break;

    case ILITEK_IOCTL_TP_FW_VER:
        res = ilitek_config_read_ic_info(ILITEK_FW_INFO);
        if (res < 0) {
            ilitek_err("Failed to get firmware version\n");
        } else {
            res = copy_to_user((u8 *) arg, p_cfg->fw_ver.data, p_pro->infos[ILITEK_FW_INFO].len);
            if (res < 0) {
                ilitek_err("Failed to copy firmware version to user space\n");
            }
        }
        break;

    case ILITEK_IOCTL_TP_PL_VER:
        res = ilitek_config_read_ic_info(ILITEK_PRO_INFO);
        if (res < 0) {
            ilitek_err("Failed to get protocol version\n");
        } else {
            res = copy_to_user((u8 *) arg, p_cfg->pro_ver.data, p_pro->infos[ILITEK_PRO_INFO].len);
            if (res < 0) {
                ilitek_err("Failed to copy protocol version to user space\n");
            }
        }
        break;

    case ILITEK_IOCTL_TP_CORE_VER:
        res = ilitek_config_read_ic_info(ILITEK_CORE_INFO);
        if (res < 0) {
            ilitek_err("Failed to get core version\n");
        } else {
            res = copy_to_user((u8 *) arg, p_cfg->core_ver.data, p_pro->infos[ILITEK_CORE_INFO].len);
            if (res < 0) {
                ilitek_err("Failed to copy core version to user space\n");
            }
        }
        break;

    case ILITEK_IOCTL_TP_DRV_VER:
        length = sprintf(szBuf, "%s", DRIVER_VERSION);
        if (!length) {
            ilitek_err("Failed to convert driver version from definiation\n");
        } else {
            res = copy_to_user((u8 *) arg, szBuf, length);
            if (res < 0) {
                ilitek_err("Failed to copy driver ver to user space\n");
            }
        }
        break;

    case ILITEK_IOCTL_TP_CHIP_ID:
        res = ilitek_config_get_chip_id(0);

        id_to_user = p_cfg->chip_info->chip_id << 16 | p_cfg->chip_info->chip_type;

        res = copy_to_user((u32 *) arg, &id_to_user, sizeof(u32));
        if (res < 0) {
            ilitek_err("Failed to copy chip id to user space\n");
        }
        break;

    case ILITEK_IOCTL_TP_NETLINK_CTRL:
        res = copy_from_user(szBuf, (u8 *) arg, 1);
        if (res < 0) {
            ilitek_err("Failed to copy data from user space\n");
        } else {
            if (szBuf[0]) {
                g_ilitek_ts->isEnableNetlink = true;
                ilitek_debug(DEBUG_IOCTL, "Netlink has been enabled\n");
            } else {
                g_ilitek_ts->isEnableNetlink = false;
                ilitek_debug(DEBUG_IOCTL, "Netlink has been disabled\n");
            }
        }
        break;

    case ILITEK_IOCTL_TP_NETLINK_STATUS:
        ilitek_debug(DEBUG_IOCTL, "Netlink is enabled : %d\n", g_ilitek_ts->isEnableNetlink);
        res = copy_to_user((int *)arg, &g_ilitek_ts->isEnableNetlink, sizeof(g_ilitek_ts->isEnableNetlink));
        if (res < 0) {
            ilitek_err("Failed to copy chip id to user space\n");
        }
        break;

    case ILITEK_IOCTL_TP_MODE_CTRL:
        res = copy_from_user(szBuf, (u8 *) arg, 4);
        if (res < 0) {
            ilitek_err("Failed to copy data from user space\n");
        } else {
            ilitek_config_disable_report_irq();
            ilitek_config_mode_ctrl(szBuf[0], szBuf);
            ilitek_config_enable_report_irq();
        }
        break;

    case ILITEK_IOCTL_TP_MODE_STATUS:
        ilitek_debug(DEBUG_IOCTL, "Current firmware mode : %d", p_rpt->fw_mode);
        res = copy_to_user((int *)arg, &p_rpt->fw_mode, sizeof(p_rpt->fw_mode));
        if (res < 0) {
            ilitek_err("Failed to copy chip id to user space\n");
        }
        break;

    default:
        res = -ENOTTY;
        break;
    }

    return res;
}

static struct proc_dir_entry *proc_dir_ilitek;

static struct file_operations proc_ioctl_fops = {
    .unlocked_ioctl = ilitek_proc_ioctl,
    //.read = ilitek_proc_ioctl_read,
    .write = ilitek_proc_ioctl_write,
};

static struct file_operations proc_fw_process_fops = {
    .read = ilitek_proc_fw_process_read,
};

static struct file_operations proc_fw_upgrade_fops = {
    .read = ilitek_proc_fw_upgrade_read,
};

static struct file_operations proc_iram_upgrade_fops = {
    .read = ilitek_proc_iram_upgrade_read,
};

static struct file_operations proc_gesture_fops = {
    .write = ilitek_proc_gesture_write,
    .read = ilitek_proc_gesture_read,
};

#ifdef BATTERY_CHECK
static struct file_operations proc_check_battery_fops = {
    .write = ilitek_proc_check_battery_write,
    .read = ilitek_proc_check_battery_read,
};
#endif

static struct file_operations proc_debug_level_fops = {
    .write = ilitek_proc_debug_level_write,
    .read = ilitek_proc_debug_level_read,
};

static struct file_operations proc_mp_test_fops = {
    .write = ilitek_proc_mp_test_write,
    .read = ilitek_proc_mp_test_read,
};

static struct file_operations proc_debug_message_fops = {
    .write = ilitek_proc_debug_message_write,
    .read = ilitek_proc_debug_message_read,
};

static struct file_operations proc_debug_message_switch_fops = {
    .read = ilitek_proc_debug_switch_read,
};

/**
 * This struct lists all file nodes will be created under /proc filesystem.
 *
 * Before creating a node that you want, declaring its file_operations structure
 * is necessary. After that, puts the structure into proc_table, defines its
 * node's name in the same row, and the init function lterates the table and
 * creates all nodes under /proc.
 *
 */
typedef struct {
    char *name;
    struct proc_dir_entry *node;
    struct file_operations *fops;
    bool isCreated;
} proc_node_t;

proc_node_t proc_table[] = {
    {"ioctl", NULL, &proc_ioctl_fops, false},
    {"fw_process", NULL, &proc_fw_process_fops, false},
    {"fw_upgrade", NULL, &proc_fw_upgrade_fops, false},
    {"iram_upgrade", NULL, &proc_iram_upgrade_fops, false},
    {"gesture", NULL, &proc_gesture_fops, false},
#ifdef BATTERY_CHECK
    {"check_battery", NULL, &proc_check_battery_fops, false},
#endif
    {"debug_level", NULL, &proc_debug_level_fops, false},
    {"mp_test", NULL, &proc_mp_test_fops, false},
    {"debug_message", NULL, &proc_debug_message_fops, false},
    {"debug_message_switch", NULL, &proc_debug_message_switch_fops, false},
};

#define NETLINK_USER 21
struct sock *_gNetLinkSkb;
struct nlmsghdr *_gNetLinkHead;
struct sk_buff *_gSkbOut;
int _gPID;

void netlink_reply_msg(void *raw, int size)
{
    int res;
    int msg_size = size;
    u8 *data = (u8 *) raw;

    ilitek_debug(DEBUG_NETLINK, "The size of data being sent to user = %d\n", msg_size);
    ilitek_debug(DEBUG_NETLINK, "pid = %d\n", _gPID);
    ilitek_debug(DEBUG_NETLINK, "Netlink is enable = %d\n", g_ilitek_ts->isEnableNetlink);

    if (g_ilitek_ts->isEnableNetlink) {
        _gSkbOut = nlmsg_new(msg_size, 0);

        if (!_gSkbOut) {
            ilitek_err("Failed to allocate new skb\n");
            return;
        }

        _gNetLinkHead = nlmsg_put(_gSkbOut, 0, 0, NLMSG_DONE, msg_size, 0);
        NETLINK_CB(_gSkbOut).dst_group = 0;    /* not in mcast group */

        /* strncpy(NLMSG_DATA(_gNetLinkHead), data, msg_size); */
        memcpy(nlmsg_data(_gNetLinkHead), data, msg_size);

        res = nlmsg_unicast(_gNetLinkSkb, _gSkbOut, _gPID);
        if (res < 0)
            ilitek_err("Failed to send data back to user\n");
    }
}

static void netlink_recv_msg(struct sk_buff *skb)
{
    _gPID = 0;

    ilitek_debug(DEBUG_NETLINK, "Netlink is enable = %d\n", g_ilitek_ts->isEnableNetlink);

    _gNetLinkHead = (struct nlmsghdr *)skb->data;

    ilitek_debug(DEBUG_NETLINK, "Received a request from client: %s, %d\n",
        (char *)NLMSG_DATA(_gNetLinkHead), (int)strlen((char *)NLMSG_DATA(_gNetLinkHead)));

    /* pid of sending process */
    _gPID = _gNetLinkHead->nlmsg_pid;

    ilitek_debug(DEBUG_NETLINK, "the pid of sending process = %d\n", _gPID);

    /* TODO: may do something if there's not receiving msg from user. */
    if (_gPID != 0) {
        ilitek_err("The channel of Netlink has been established successfully !\n");
        g_ilitek_ts->isEnableNetlink = true;
    } else {
        ilitek_err("Failed to establish the channel between kernel and user space\n");
        g_ilitek_ts->isEnableNetlink = false;
    }
}

static int netlink_init(void)
{
    int res = 0;

#if KERNEL_VERSION(3, 4, 0) > LINUX_VERSION_CODE
    _gNetLinkSkb = netlink_kernel_create(&init_net, NETLINK_USER, netlink_recv_msg, NULL, THIS_MODULE);
#else
    struct netlink_kernel_cfg cfg = {
        .input = netlink_recv_msg,
    };

    _gNetLinkSkb = netlink_kernel_create(&init_net, NETLINK_USER, &cfg);
#endif

    ilitek_info("Initialise Netlink and create its socket\n");

    if (!_gNetLinkSkb) {
        ilitek_err("Failed to create nelink socket\n");
        res = -EFAULT;
    }

    return res;
}

int ilitek_proc_init(void)
{
    int i = 0, res = 0;

    proc_dir_ilitek = proc_mkdir("ilitek", NULL);

    for (; i < ARRAY_SIZE(proc_table); i++) {
        /* others no write permission */
        proc_table[i].node = proc_create(proc_table[i].name, S_IRUGO | S_IWUSR| S_IWGRP,
            proc_dir_ilitek, proc_table[i].fops);

        if (proc_table[i].node == NULL) {
            proc_table[i].isCreated = false;
            ilitek_err("Failed to create %s under /proc\n", proc_table[i].name);
            res = -ENODEV;
        } else {
            proc_table[i].isCreated = true;
            ilitek_info("Succeed to create %s under /proc\n", proc_table[i].name);
        }
    }

    netlink_init();

    return res;
}

void ilitek_proc_remove(void)
{
    int i = 0;

    for (; i < ARRAY_SIZE(proc_table); i++) {
        if (proc_table[i].isCreated == true) {
            ilitek_info("Removed %s under /proc\n", proc_table[i].name);
            remove_proc_entry(proc_table[i].name, proc_dir_ilitek);
        }
    }

    remove_proc_entry("ilitek", NULL);
    netlink_kernel_release(_gNetLinkSkb);

}

