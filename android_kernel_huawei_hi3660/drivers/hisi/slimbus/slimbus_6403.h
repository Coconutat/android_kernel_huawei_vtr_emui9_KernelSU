/*
 *	slimbus is a kernel driver which is used to manager SLIMbus devices
 *	Copyright (C) 2014	Hisilicon

 *	This program is free software: you can redistribute it and/or modify
 *	it under the terms of the GNU General Public License as published by
 *	the Free Software Foundation, either version 3 of the License, or
 *	(at your option) any later version.

 *	This program is distributed in the hope that it will be useful,
 *	but WITHOUT ANY WARRANTY; without even the implied warranty of
 *	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *	GNU General Public License for more details.

 *	You should have received a copy of the GNU General Public License
 *	along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 */

#ifndef _SLIMBUS_6403_H_
#define _SLIMBUS_6403_H_


#include "slimbus_types.h"

extern int create_hi6403_slimbus_device(slimbus_device_info_t **device);

extern void slimbus_hi6403_param_init(slimbus_device_info_t *dev);

extern void slimbus_hi6403_param_update(
			slimbus_device_info_t *dev,
			uint32_t track_type,
			slimbus_track_param_t *params);

extern int slimbus_hi6403_param_set(
			slimbus_device_info_t *dev,
			uint32_t track_type,
			slimbus_track_param_t *params);

extern void slimbus_hi6403_get_st_params(slimbus_sound_trigger_params_t *params);
#endif

