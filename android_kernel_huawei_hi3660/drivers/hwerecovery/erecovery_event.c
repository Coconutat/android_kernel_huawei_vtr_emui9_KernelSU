/**********************************************************
 * Filename:    erecovery_event.c
 *
 * Discription: Interfaces implementation for sending hung event
                from kernel
 *
 * Copyright: (C) 2017 huawei.
 *
 * Author:
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
#include "chipset_common/hwerecovery/erecovery.h"
#include "erecovery_common.h"
#include "erecovery_transtation.h"

long erecovery_report(erecovery_eventobj* eventdata) {
    long ret = 0;
    erecovery_write_event evt = {0};
    if (in_atomic() || in_interrupt()) {
        ERECOVERY_ERROR("can not report event in interrupt context");
        return -EINVAL;
    }
    if (!eventdata) {
        ERECOVERY_ERROR("eventdata is null");
        return -EINVAL;
    }
    memset(&evt,0,sizeof(evt));
    evt.magic = ERECOVERY_MAGIC_NUM;
    evt.ere_obj = *eventdata;
    ret = erecovery_write_event_kernel(&evt);
    ERECOVERY_INFO("erecovery report event from kernel");
    return ret;

}

EXPORT_SYMBOL(erecovery_report);


