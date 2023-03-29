/*
 * hisi_bootup_keypoint for guide post
 *
 * Copyright (c) 2013 Huawei Technologies CO., Ltd.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#ifndef __BOOT_STAGE_POINT_H__
#define __BOOT_STAGE_POINT_H__

#include <mntn_public_interface.h>

#define FPGA 1
void set_boot_keypoint(u32 value);
u32 get_boot_keypoint(void);
u32 get_last_boot_keypoint(void);
#endif
