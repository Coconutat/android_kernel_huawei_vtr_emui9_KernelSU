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
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/slab.h>
#include <linux/platform_device.h>
#include <linux/of.h>
#include <linux/delay.h>

#include "csmi.h"
#include "slimbus_drv.h"
#include "slimbus.h"
#include "slimbus_64xx.h"
#include <dsm/dsm_pub.h>

/*lint -e750 -e730 -e785 -e574*/
#define LOG_TAG "Slimbus_64xx"

void slimbus_hi64xx_set_para_pr(
			slimbus_presence_rate_t *pr_table,
			uint32_t  track_type,
			slimbus_track_param_t *params)
{
	if (params) {
		if (params->rate == SLIMBUS_SAMPLE_RATE_8K) {
			pr_table[track_type] = SLIMBUS_PR_8K;
		} else if (params->rate == SLIMBUS_SAMPLE_RATE_16K) {
			pr_table[track_type] = SLIMBUS_PR_16K;
		} else if (params->rate == SLIMBUS_SAMPLE_RATE_32K) {
			pr_table[track_type] = SLIMBUS_PR_32K;
		} else if (params->rate == SLIMBUS_SAMPLE_RATE_48K) {
			pr_table[track_type] = SLIMBUS_PR_48K;
		} else if (params->rate == SLIMBUS_SAMPLE_RATE_96K) {
			pr_table[track_type] = SLIMBUS_PR_96K;
		} else if (params->rate == SLIMBUS_SAMPLE_RATE_192K) {
			pr_table[track_type] = SLIMBUS_PR_192K;
		} else if (params->rate == SLIMBUS_SAMPLE_RATE_768K) {
			pr_table[track_type] = SLIMBUS_PR_768K;
		} else {
			pr_err("[%s:%d] sample rate is invalid: %d\n", __FUNCTION__, __LINE__, params->rate);
		}
	}

	return;
}

void release_hi64xx_slimbus_device(slimbus_device_info_t *device)
{
	if (NULL == device) {
		pr_err("device is null");
		return;
	}

	if (device->slimbus_64xx_para != NULL) {
		kfree(device->slimbus_64xx_para);
		device->slimbus_64xx_para= NULL;
	}

	mutex_destroy(&(device->rw_mutex));
	mutex_destroy(&(device->track_mutex));

	kfree(device);
	device = NULL;
}

