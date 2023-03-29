/*
 * Copyright (C) 2018 Huawei Device Co., Ltd.
 * This file is licensed under GPL.
 */

#ifndef VENDOR_EXCEPTION_H
#define VENDOR_EXCEPTION_H

#ifdef CONFIG_HUAWEI_VENDOR_EXCEPTION
#include <linux/hisi/rdr_hisi_platform.h>
#include <linux/delay.h>

enum {
	VENDOR_MODID_AP_S_CFI = MODID_AP_S_VENDOR_BEGIN + 0x00000,
};

#define VENDOR_EXCEPTION(modid, arg1, arg2) \
	do { \
		set_exception_info((u64)__builtin_return_address(0)); \
		rdr_system_error(modid, arg1, arg2); \
		msleep(-1U); \
	} while (0)
#endif

#endif
