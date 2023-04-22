/**********************************************************
 * Filename:    zrhung_ioctl.c
 *
 * Discription: ioctl implementaion for zerohung
 *
 * Copyright: (C) 2017 huawei.
 *
 * Author: zhaochenxiao(00344580) zhangliang(00175161)
 *
**********************************************************/
#include <linux/sched.h>
#include <linux/module.h>
#include <linux/poll.h>
#include <linux/slab.h>
#include <linux/time.h>
#include <linux/vmalloc.h>
#include <linux/aio.h>
#include <uapi/linux/uio.h>
#include <asm/ioctls.h>
#include <chipset_common/hwlogger/hw_logger.h>
#include "chipset_common/hwzrhung/zrhung.h"
#include "zrhung_common.h"
#include "zrhung_config.h"
#include "zrhung_transtation.h"

long zrhung_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
	long ret = ZRHUNG_CMD_INVALID;

	if(cmd < LOGGER_WRITE_HEVENT || cmd >= LOGGER_CMD_MAX)
		return ret;

	switch (cmd) {
	case LOGGER_WRITE_HEVENT:			// write event
		ret = htrans_write_event((void*)arg);
		break;
	case LOGGER_READ_HEVENT:			// read event
		ret = htrans_read_event((void*)arg);
		break;
	case LOGGER_GET_HLASTWORD:			// read last word
		ret = htrans_read_lastword((void*)arg);
		break;
	case LOGGER_SET_HCFG:
		ret = hcfgk_set_cfg(file, (void*)arg);
		break;
	case LOGGER_GET_HCFG:
		ret = hcfgk_ioctl_get_cfg(file, (void*)arg);
		break;
	case LOGGER_SET_HCFG_FLAG:
		ret = hcfgk_set_cfg_flag(file, (void*)arg);
		break;
	case LOGGER_GET_HCFG_FLAG:
		ret = hcfgk_get_cfg_flag(file, (void*)arg);
		break;
	case LOGGER_SET_FEATURE:
		ret = hcfgk_set_feature(file, (void*)arg);
	default:
		break;
	}

	return ret;
}
EXPORT_SYMBOL(zrhung_ioctl);
