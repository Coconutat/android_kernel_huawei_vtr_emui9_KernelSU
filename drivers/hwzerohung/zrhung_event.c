/**********************************************************
 * Filename:    zrhung_event.c
 *
 * Discription: Interfaces implementation for sending hung event
                from kernel
 *
 * Copyright: (C) 2017 huawei.
 *
 * Author: huangyu(00381502) zhangliang(00175161)
 *
**********************************************************/
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/types.h>
#include <linux/errno.h>
#include <linux/vmalloc.h>
#include <linux/fs.h>
#include <linux/string.h>
#include <linux/slab.h>
#include <linux/version.h>
#include <linux/ioctl.h>
#include <asm/uaccess.h>
#include "huawei_platform/log/hw_log.h"
#include "chipset_common/hwzrhung/zrhung.h"
#include "zrhung_common.h"
#include "zrhung_transtation.h"

#define HWLOG_TAG    zrhung
#define BUF_SIZE  (sizeof(zrhung_write_event)+ZRHUNG_CMD_LEN_MAX+ZRHUNG_MSG_LEN_MAX+2)

HWLOG_REGIST();
static uint8_t g_buf[BUF_SIZE];
static DEFINE_SPINLOCK(lock);

int zrhung_send_event(zrhung_wp_id id, const char* cmd_buf, const char* msg_buf)
{
    int ret     = 0;
    int cmd_len = 0;
    int msg_len = 0;
    char *p     = NULL;
    char *out_buf   = NULL;
    int total_len   = sizeof(zrhung_write_event);
    zrhung_write_event evt = {0};
    unsigned long flags;

    /* now we support atomic context */
    /*if (in_atomic() || in_interrupt()) {
        hwlog_err("can not zrhung_send_event in interrupt context");
        return -EINVAL;
    }*/
    memset(&evt, 0, sizeof(evt));
    if (zrhung_is_id_valid(id) < 0) {
        hwlog_err("Bad watchpoint id");
        return -EINVAL;
    }

    if (cmd_buf)
        cmd_len = strlen(cmd_buf);
    if (cmd_len > ZRHUNG_CMD_LEN_MAX) {
        hwlog_err("watchpoint cmd too long");
        return -EINVAL;
    }
    total_len += cmd_len + 1;

    if (msg_buf)
        msg_len = strlen(msg_buf);
    if (msg_len > ZRHUNG_MSG_LEN_MAX) {
        hwlog_err("watchpoint msg buffer too long");
        return -EINVAL;
    }
    total_len += msg_len + 1;

    spin_lock_irqsave(&lock, flags);
    out_buf = g_buf;

    /* construct the message */
    evt.magic   = MAGIC_NUM;
    evt.len     = total_len;
    evt.wp_id   = id;
    evt.cmd_len = cmd_len + 1;
    evt.msg_len = msg_len + 1;

    memset(out_buf, 0, total_len);
    p  = out_buf;
    memcpy(p, &evt, sizeof(evt));
    p += sizeof(evt);

    if (cmd_len > 0) {
        memcpy(p, cmd_buf, cmd_len);
    }
    p += cmd_len;
    *p = 0;
    p++;

    if (msg_buf) {
        memcpy(p, msg_buf, msg_len);
    }
    p += msg_len;
    *p = 0;

    /* send the message */
    ret = htrans_write_event_kernel(out_buf);
    spin_unlock_irqrestore(&lock, flags);

    hwlog_info("zrhung send event from kernel: wp=%d, ret=%d", evt.wp_id, ret);

    return ret;
}

EXPORT_SYMBOL(zrhung_send_event);
