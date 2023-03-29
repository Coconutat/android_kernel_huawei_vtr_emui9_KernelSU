/*
 * anc_max14744.h -- anc headset driver
 *
 * Copyright (c) 2014 Huawei Technologies CO., Ltd.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */
#ifndef ANC_DEFAULT
#define ANC_DEFAULT

#include "huawei_platform/audio/anc_hs_interface.h"

#ifdef CONFIG_ANC_DEFAULT
void anc_max14744_refresh_headset_type(int headset_type);
#else
void anc_max14744_refresh_headset_type(int headset_type)
{
	return;
}
#endif //CONFIG_ANC_DEFAULT

#endif //ANC_DEFAULT
