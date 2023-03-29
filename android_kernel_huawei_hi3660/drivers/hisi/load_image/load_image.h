/*
 * load_image.h
 *
 * Hisilicon image load driver head file.
 *
 * Copyright (c) 2001-2021, Huawei Tech. Co., Ltd. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 and
 * only version 2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */
#ifndef _LOAD_IMAGE_H_
#define _LOAD_IMAGE_H_
#include <linux/hisi/hisi_load_image.h>
#ifdef __cplusplus
extern "C"
{
#endif

#define sec_print_err(fmt, ...)    (printk(KERN_ERR "[sec]: <%s> "fmt, __FUNCTION__, ##__VA_ARGS__))
#define sec_print_info(fmt, ...)   (printk(KERN_INFO "[sec]: <%s> "fmt, __FUNCTION__, ##__VA_ARGS__))
#define SEC_OK      			(0)
#define SEC_ERROR  			(-1)
#define SEC_FILE_NO_EXSIT		(-2)
#define VRL_SIZE			(0x1000)

#define BSP_RESET_NOTIFY_REPLY_OK	(0)
#define BSP_RESET_NOTIFY_SEND_FAILED	(-1)
#define BSP_RESET_NOTIFY_TIMEOUT	(-2)

enum {
	PrimVRL = 0x0,
	BackVRL = 0x1,
};

typedef enum {
	BSP_CCORE = 0,
	BSP_HIFI,
	BSP_LPM3,
	BSP_BBE,
	BSP_CDSP,
	BSP_CCORE_TAS,
	BSP_CCORE_WAS,
	BSP_CCORE_CAS,
	BSP_BUTT
} BSP_CORE_TYPE_E;


int bsp_reset_core_notify(BSP_CORE_TYPE_E ecoretype, unsigned int cmdtype, unsigned int timeout_ms, unsigned int *retval);

#ifdef __cplusplus
}
#endif
#endif

