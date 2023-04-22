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

#ifndef _SLIMBUS_64xx_H_
#define _SLIMBUS_64xx_H_

#include "slimbus_types.h"

/* slimbus soc side port definition */
typedef enum
{
       AUDIO_PLAY_SOC_LEFT_PORT      = 0,
       AUDIO_PLAY_SOC_RIGHT_PORT     = 1,
       AUDIO_CAPTURE_SOC_LEFT_PORT   = 2,
       AUDIO_CAPTURE_SOC_RIGHT_PORT  = 3,
       IMAGE_DOWNLOAD_SOC_PORT       = 4,
       SUPER_PLAY_SOC_RIGHT_PORT     = 5,
       BT_CAPTURE_SOC_LEFT_PORT      = 6,
       BT_CAPTURE_SOC_RIGHT_PORT     = 7,
       VOICE_DOWN_SOC_LEFT_PORT      = 8,
       VOICE_DOWN_SOC_RIGHT_PORT     = 9,
       VOICE_UP_SOC_MIC1_PORT        = 10,
       VOICE_UP_SOC_MIC2_PORT        = 11,
       VOICE_UP_SOC_MIC3_PORT        = 12,
       VOICE_UP_SOC_MIC4_PORT        = 13,
       VOICE_SOC_ECREF_PORT          = 14,
       AUDIO_SOC_ECREF_PORT          = 15,
} slimbus_soc_port_t;

/* slimbus device side port definition */
typedef enum
{
       AUDIO_PLAY_64XX_LEFT_PORT     = 0,
       AUDIO_PLAY_64XX_RIGHT_PORT    = 1,
       AUDIO_CAPTURE_64XX_LEFT_PORT  = 2,
       AUDIO_CAPTURE_64XX_RIGHT_PORT = 3,
       IMAGE_DOWNLOAD_64XX_PORT      = 4,
       SUPER_PLAY_64XX_PORT          = 5,
       BT_CAPTURE_64XX_LEFT_PORT     = 6,
       BT_CAPTURE_64XX_RIGHT_PORT    = 7,
       VOICE_DOWN_64XX_LEFT_PORT     = 8,
       VOICE_DOWN_64XX_RIGHT_PORT    = 9,
       VOICE_UP_64XX_MIC1_PORT       = 10,
       VOICE_UP_64XX_MIC2_PORT       = 11,
       VOICE_UP_64XX_MIC3_PORT       = 12,
       VOICE_UP_64XX_MIC4_PORT       = 13,
       VOICE_64XX_ECREF_PORT         = 14,
       AUDIO_64XX_ECREF_PORT         = 15,
} slimbus_64xx_port_t;

extern void release_hi64xx_slimbus_device(slimbus_device_info_t *dev);

extern void slimbus_hi64xx_set_para_pr(
			slimbus_presence_rate_t *pr_table,
			uint32_t  track_type,
			slimbus_track_param_t *params);

#endif

