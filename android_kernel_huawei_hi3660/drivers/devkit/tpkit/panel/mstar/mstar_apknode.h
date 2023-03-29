/*
 * Copyright (C) 2006-2017 ILITEK TECHNOLOGY CORP.
 *
 * Description: ILITEK I2C touchscreen driver for linux platform.
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
 * along with this program; if not, see the file COPYING, or write
 * to the Free Software Foundation, Inc.,
 * 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 *
 * Author: Johnson Yeh
 * Maintain: Luca Hsu, Tigers Huang, Dicky Chiang
 */

#ifndef __MSTAR_APKNODE_H__
#define __MSTAR_APKNODE_H__

#include "mstar_common.h"

extern void mstar_apknode_delete_jni_msg(void);
extern void mstar_apknode_create_jni_msg(void);
extern void mstar_apknode_get_glove_info(u8 * pGloveMode);
extern void mstar_apknode_close_leath_sheath(void);
extern void mstar_apknode_open_glove(void);
extern void mstar_apknode_close_glove(void);
extern void mstar_apknode_open_leather_sheath(void);
extern int mstar_apknode_create_procfs(void);
extern void mstar_apknode_remove_procfs(void);
void mstar_apknode_remove_gesturenode(void);
void mstar_gesture_notify(u8 *packet);
void mstar_apknode_function_control(void);
struct apk_data_info * mstar_malloc_apknode(void);
int mstar_apknode_para_init(void);
#endif
