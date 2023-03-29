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
#include <linux/io.h>
#include <linux/fs.h>
#include <linux/init.h>
#include <linux/platform_device.h>
#include <linux/types.h>
#include <linux/delay.h>
#include <linux/clk.h>
#include <linux/regulator/consumer.h>
#include <linux/of.h>
#include <linux/slab.h>
#include <linux/pm_runtime.h>
#include <dsm_audio/dsm_audio.h>

#include "hi3630_asp_common.h"

#include "slimbus_utils.h"
#include "slimbus_drv.h"
#include "slimbus.h"
#include "slimbus_6402.h"
#include "slimbus_64xx.h"
#include "slimbus_6403.h"

/*lint -e838 -e730 -e747 -e774 -e826 -e529 -e438 -e485 -e785 -e651 -e64 -e527 -e570*/

slimbus_channel_property_t audio_playback[SLIMBUS_AUDIO_PLAYBACK_CHANNELS] = {{0,{0,},{{0,},},},};
slimbus_channel_property_t audio_capture[SLIMBUS_AUDIO_CAPTURE_MULTI_MIC_CHANNELS] = {{0,{0,},{{0,},},},};
slimbus_channel_property_t voice_down[SLIMBUS_VOICE_DOWN_CHANNELS] = {{0,{0,},{{0,},},},};
slimbus_channel_property_t voice_up[SLIMBUS_VOICE_UP_CHANNELS] = {{0,{0,},{{0,},},},};
slimbus_channel_property_t img_download[SLIMBUS_IMAGE_DOWNLOAD_CHANNELS] = {{0,{0,},{{0,},},},};
slimbus_channel_property_t ec_ref[SLIMBUS_ECREF_CHANNELS] = {{0,{0,},{{0,},},},};
slimbus_channel_property_t sound_trigger[SLIMBUS_SOUND_TRIGGER_CHANNELS] = {{0,{0,},{{0,},},},};
slimbus_channel_property_t audio_debug[SLIMBUS_DEBUG_CHANNELS] = {{0,{0,},{{0,},},},};
slimbus_channel_property_t audio_direct_playback[SLIMBUS_AUDIO_PLAYBACK_CHANNELS] = {{0,{0,},{{0,},},},};
slimbus_channel_property_t audio_fast_playback[SLIMBUS_AUDIO_PLAYBACK_CHANNELS] = {{0,{0,},{{0,},},},};

enum {
	SLIMBUS_TRACK_PLAY_ON                   = 1<<SLIMBUS_TRACK_AUDIO_PLAY,
	SLIMBUS_TRACK_CAPTURE_ON                = 1<<SLIMBUS_TRACK_AUDIO_CAPTURE,
	SLIMBUS_TRACK_VOICE_DOWN_ON             = 1<<SLIMBUS_TRACK_VOICE_DOWN,
	SLIMBUS_TRACK_VOICE_UP_ON               = 1<<SLIMBUS_TRACK_VOICE_UP,
	SLIMBUS_TRACK_IMAGE_LOAD_ON             = 1<<SLIMBUS_TRACK_IMAGE_LOAD,
	SLIMBUS_TRACK_EC_ON                     = 1<<SLIMBUS_TRACK_ECREF,
	SLIMBUS_TRACK_SOUND_TRIGGER_ON          = 1<<SLIMBUS_TRACK_SOUND_TRIGGER,
	SLIMBUS_TRACK_DEBUG_ON                  = 1<<SLIMBUS_TRACK_DEBUG,
	SLIMBUS_TRACK_DIRECT_PLAY_ON            = 1<<SLIMBUS_TRACK_DIRECT_PLAY,
	SLIMBUS_TRACK_FAST_PLAY_ON              = 1<<SLIMBUS_TRACK_FAST_PLAY,
};

struct slimbus_private_data
{
	struct device           *dev;
	void __iomem            *base_addr;          /* SoC SLIMbus base address (virtual address) */
	void __iomem            *asp_reg_base_addr;  /* asp address(virtual address) */
	void __iomem            *sctrl_base_addr;    /* sctrl address(virtual address) */
	uint32_t                asp_power_state_offset;
	uint32_t                asp_clk_state_offset;
	int                     irq;                 /* SoC SLIMbus irq num */
	struct regulator        *regu_asp;           /* regulator of asp */
	struct pinctrl          *pctrl;
	struct pinctrl_state    *pin_default;        /* pinctrl default */
	struct pinctrl_state    *pin_idle;           /* pinctrl idle */
	struct clk              *pmu_audio_clk;      /* codec 19.2M clk */
	struct clk              *asp_subsys_clk;
	uint32_t                portstate;
	slimbus_framer_type_t   framerstate;
	slimbus_framer_type_t   lastframer;
	platform_type_t         platform_type;
	slimbus_device_type_t   device_type;
	struct timer_list       timer;
	bool                    slimbus_dynamic_freq_enable;
	slimbus_device_ops_t    *dev_ops;
	slimbus_track_config_t  *track_config_table;
	uint32_t                slimbus_track_max;
	bool                    pm_runtime_support;
	bool                    switch_framer_disable;
	bool                    *track_state;
};

static struct slimbus_private_data *pdata;
slimbus_device_info_t *slimbus_devices[SLIMBUS_DEVICE_NUM] = {0};
uint32_t slimbus_log_count = 300;
uint32_t slimbus_rdwrerr_times = 0;
slimbus_device_info_t *slimbus_dev = NULL;

bool hi6402_soundtrigger_if1_used = false;

slimbus_track_config_t track_config_table[SLIMBUS_TRACK_MAX] = {
	/* play back */
	{
		.params.channels = SLIMBUS_AUDIO_PLAYBACK_CHANNELS,
		.params.rate = SLIMBUS_SAMPLE_RATE_48K,
		.params.callback = NULL,
		.channel_pro = &audio_playback[0],
	},
	/* capture */
	{
		.params.channels = SLIMBUS_AUDIO_CAPTURE_MULTI_MIC_CHANNELS,
		.params.rate = SLIMBUS_SAMPLE_RATE_48K,
		.params.callback = NULL,
		.channel_pro = &audio_capture[0],
	},
	/* voice down */
	{
		.params.channels = SLIMBUS_VOICE_DOWN_CHANNELS,
		.params.rate = SLIMBUS_SAMPLE_RATE_8K,
		.params.callback = NULL,
		.channel_pro = &voice_down[0],
	},
	/* voice up */
	{
		.params.channels = SLIMBUS_VOICE_UP_CHANNELS,
		.params.rate = SLIMBUS_SAMPLE_RATE_8K,
		.params.callback = NULL,
		.channel_pro = &voice_up[0],
	},
	/* img download */
	{
		.params.channels = SLIMBUS_IMAGE_DOWNLOAD_CHANNELS,
		.params.rate = SLIMBUS_SAMPLE_RATE_768K,
		.params.callback = NULL,
		.channel_pro = &img_download[0],
	},
	/* ec */
	{
		.params.channels = SLIMBUS_ECREF_CHANNELS,
		.params.rate = SLIMBUS_SAMPLE_RATE_48K,
		.params.callback = NULL,
		.channel_pro = &ec_ref[0],
	},
	/* sound trigger */
	{
		.params.channels = SLIMBUS_SOUND_TRIGGER_CHANNELS,
		.params.rate = SLIMBUS_SAMPLE_RATE_192K,
		.params.callback = NULL,
		.channel_pro = &sound_trigger[0],
	},
	/* debug */
	{
		.params.channels = SLIMBUS_DEBUG_CHANNELS,
		.params.rate = SLIMBUS_SAMPLE_RATE_192K,
		.params.callback = NULL,
		.channel_pro = &audio_debug[0],
	},
	/* direct play */
	{
		.params.channels = SLIMBUS_AUDIO_PLAYBACK_CHANNELS,
		.params.rate = SLIMBUS_SAMPLE_RATE_192K,
		.params.callback = NULL,
		.channel_pro = &audio_direct_playback[0],
	},
	/* fast play */
	{
		.params.channels = SLIMBUS_AUDIO_PLAYBACK_CHANNELS,
		.params.rate = SLIMBUS_SAMPLE_RATE_48K,
		.params.callback = NULL,
		.channel_pro = &audio_fast_playback[0],
	},

};

/*
 * SLIMbus bus configuration
 */
slimbus_bus_config_t bus_config[SLIMBUS_BUS_CONFIG_MAX] = {
	/* normal run */
	{
		.sm = SLIMBUS_SM_1_CSW_32_SL,    /* control space:4; subframe length:32; */
		.cg = SLIMBUS_CG_10,             /* clock gear*/
		.rf = SLIMBUS_RF_1,              /* root frequency: 24.576MHZ*/
	},
	/* img download */
	{
		.sm = SLIMBUS_SM_3_CSW_8_SL,     /* control space:3; subframe length:8; */
		.cg = SLIMBUS_CG_10,             /* clock gear*/
		.rf = SLIMBUS_RF_1,              /* root frequency: 24.576MHZ*/
	},
	/* switch framer */
	{
		.sm = SLIMBUS_SM_1_CSW_32_SL,    /* control space:4; subframe length:32; */
		.cg = SLIMBUS_CG_10,             /* clock gear*/
		.rf = SLIMBUS_RF_1,              /* root frequency: 24.576MHZ*/
	},
	/* reg write img download */
	{
		.sm = SLIMBUS_SM_8_CSW_8_SL,     /* control space:8; subframe length:8; */
		.cg = SLIMBUS_CG_10,             /* clock gear*/
		.rf = SLIMBUS_RF_1,              /* root frequency: 24.576MHZ*/
	},
};


uint32_t slimbus_logcount_get(void)
{
	return slimbus_log_count;
}

void slimbus_logcount_set(uint32_t count)
{
	slimbus_log_count = count;
}

uint32_t slimbus_logtimes_get(void)
{
	return slimbus_rdwrerr_times;
}

void slimbus_logtimes_set(uint32_t times)
{
	slimbus_rdwrerr_times = times;
}

int slimbus_element_read(
				slimbus_device_info_t *dev,
				int32_t byte_address,
				slimbus_slice_size_t slice_size,
				void *value)
{
	static int slimbus_dmd_flag = 1;
	uint32_t reg_page = byte_address & (~0xff);
	uint8_t *paddr = (uint8_t*)&byte_address;
	uint8_t ret = 0;

	if (slice_size >= SLIMBUS_SS_SLICE_BUT) {
		SLIMBUS_LIMIT_ERR("slice size is invalid, slice_size:%d\n", slice_size);
		return -EINVAL;
	}

	mutex_lock(&dev->rw_mutex);
	if (dev->page_sel_addr != reg_page) {
		ret  = slimbus_drv_element_write(dev->generic_la, HI6402_PAGE_SELECT_REG_0, SLIMBUS_SS_1_BYTE, (paddr+1));
		ret += slimbus_drv_element_write(dev->generic_la, HI6402_PAGE_SELECT_REG_1, SLIMBUS_SS_1_BYTE, (paddr+2));
		ret += slimbus_drv_element_write(dev->generic_la, HI6402_PAGE_SELECT_REG_2, SLIMBUS_SS_1_BYTE, (paddr+3));

		dev->page_sel_addr = reg_page;
	}
	ret += slimbus_drv_element_read(dev->generic_la, SLIMBUS_USER_DATA_BASE_ADDR + *paddr, slice_size, (uint8_t *)value);
	mutex_unlock(&dev->rw_mutex);

	if (ret) {
		SLIMBUS_LIMIT_ERR("read error! slice_size=%d, addr=0x%pK!\n", slice_size, (void *)(long)byte_address);
		if (1 == slimbus_dmd_flag) {
			if (audio_dsm_report_info(AUDIO_CODEC, DSM_HI6402_SLIMBUS_READ_ERR,
				"slice_size=%d, addr=0x%pK\n",slice_size, (void *)(long)byte_address) >= 0) {
				slimbus_dmd_flag = 0;
			}
		}
		return -EFAULT;
	}
	SLIMBUS_RECOVER_INFO("read recover, slice_size=%d, addr=%pK!\n", slice_size, (void *)(long)byte_address);

	return 0;
}

int slimbus_element_write(
				slimbus_device_info_t *dev,
				int32_t byte_address,
				slimbus_slice_size_t slice_size,
				void *value)
{
	uint32_t reg_page = byte_address & (~0xff);
	uint8_t *paddr = (uint8_t*)&byte_address;
	uint8_t ret = 0;

	if (slice_size >= SLIMBUS_SS_SLICE_BUT) {
		SLIMBUS_LIMIT_ERR("slice size is invalid, slice_size:%d\n", slice_size);
		return -EINVAL;
	}

	mutex_lock(&dev->rw_mutex);
	if (dev->page_sel_addr != reg_page) {
		ret  = slimbus_drv_element_write(dev->generic_la, HI6402_PAGE_SELECT_REG_0, SLIMBUS_SS_1_BYTE, (paddr+1));
		ret += slimbus_drv_element_write(dev->generic_la, HI6402_PAGE_SELECT_REG_1, SLIMBUS_SS_1_BYTE, (paddr+2));
		ret += slimbus_drv_element_write(dev->generic_la, HI6402_PAGE_SELECT_REG_2, SLIMBUS_SS_1_BYTE, (paddr+3));

		dev->page_sel_addr = reg_page;
	}
	ret += slimbus_drv_element_write(dev->generic_la, (SLIMBUS_USER_DATA_BASE_ADDR + *paddr), slice_size, (uint8_t *)value);
	mutex_unlock(&dev->rw_mutex);

	if (ret) {
		SLIMBUS_LIMIT_ERR("write error! slice_size=%d, addr=0x%pK!\n", slice_size, (void *)(long)byte_address);
		return -EFAULT;
	}
	SLIMBUS_RECOVER_INFO("write recover, slice_size=%d, addr=%pK!\n", slice_size, (void *)(long)byte_address);

	return 0;
}

unsigned int slimbus_read_1byte(unsigned int reg)
{
	static unsigned int value = 0;
	int retry_count = 0;
	static uint32_t info0 = 0xa1;
	static uint32_t info1 = 0xa2;
	static uint32_t info2 = 0xa3;
	static uint32_t info3 = 0xa4;

	if (slimbus_dev == NULL) {
		SLIMBUS_LIMIT_ERR("slimbus device not allocate!\n");
		return ENXIO;
	}

	value = 0x5A;
	do {
		slimbus_element_read(slimbus_dev, reg, SLIMBUS_SS_1_BYTE, &value);

		if (value == 0x5A) {
			SLIMBUS_LIMIT_INFO("SLIMbus read1byte retry: reg:0x%pK, val:%#x !\n", (void *)(unsigned long)reg, value);
			retry_count++;
			mdelay(1);
		}
	} while ((value == 0x5A) && (retry_count <= 3));

	if (retry_count > 0) {
		info0 = 0xa1;
		info1 = 0xa2;
		info2 = 0xa3;
		info3 = 0xa4;
		slimbus_drv_request_info(0x21, 0x400, SLIMBUS_SS_1_BYTE, &info0);
		slimbus_drv_request_info(0x40, 0x400, SLIMBUS_SS_1_BYTE, &info1);
		slimbus_drv_request_info(0x20, 0x400, SLIMBUS_SS_1_BYTE, &info2);
		slimbus_drv_request_info(0x41, 0x400, SLIMBUS_SS_1_BYTE, &info3);
		SLIMBUS_LIMIT_INFO("SLIMbus info: %#x, %#x, %#x, %#x \n", info0, info1, info2, info3);
	}
	return value;
}

unsigned int slimbus_read_4byte(unsigned int reg)
{
	static unsigned int value = 0;
	static int valbyte = 0;
	int retry_count = 0;

	if (slimbus_dev == NULL) {
		SLIMBUS_LIMIT_ERR("slimbus device not allocate!\n");
		return -ENXIO;
	}

	value = 0x6A6A6A6A;
	valbyte = 0;
	do {
		slimbus_element_read(slimbus_dev, reg, SLIMBUS_SS_4_BYTES, &value);
		slimbus_element_read(slimbus_dev, 0x20007022, SLIMBUS_SS_1_BYTE, &valbyte);
		slimbus_element_read(slimbus_dev, 0x20007023, SLIMBUS_SS_4_BYTES, &value);

		if (value == 0x6A6A6A6A) {
			SLIMBUS_LIMIT_INFO("SLIMbus read4byte retry: reg:0x%pK, val:%#x !\n", (void *)(unsigned long)reg, value);
			retry_count++;
			mdelay(1);
		}
	} while ((value == 0x6A6A6A6A) && (retry_count <= 3));

	return value;
}

void slimbus_write_1byte(unsigned int reg, unsigned int val)
{
	if (slimbus_dev == NULL) {
		SLIMBUS_LIMIT_ERR("slimbus device not allocate!\n");
		return;
	}

	slimbus_element_write(slimbus_dev, reg, SLIMBUS_SS_1_BYTE, &val);
}

void slimbus_write_4byte(unsigned int reg, unsigned int val)
{
	if (slimbus_dev == NULL) {
		SLIMBUS_LIMIT_ERR("slimbus device not allocate!\n");
		return;
	}

	slimbus_element_write(slimbus_dev, reg, SLIMBUS_SS_4_BYTES, &val);
}

void slimbus_read_pageaddr(void)
{
	static int page0 = 0;
	static int page1 = 0;
	static int page2 = 0;

	if (slimbus_dev == NULL) {
		SLIMBUS_LIMIT_ERR("slimbus device not allocate!\n");
		return;
	}

	page0 = 0xA5;
	page1 = 0xA5;
	page2 = 0xA5;
	mutex_lock(&slimbus_dev->rw_mutex);
	slimbus_drv_element_read(slimbus_dev->generic_la, HI6402_PAGE_SELECT_REG_0, SLIMBUS_SS_1_BYTE, (uint8_t *)&page0);
	slimbus_drv_element_read(slimbus_dev->generic_la, HI6402_PAGE_SELECT_REG_1, SLIMBUS_SS_1_BYTE, (uint8_t *)&page1);
	slimbus_drv_element_read(slimbus_dev->generic_la, HI6402_PAGE_SELECT_REG_2, SLIMBUS_SS_1_BYTE, (uint8_t *)&page2);
	mutex_unlock(&slimbus_dev->rw_mutex);

	pr_info("[%s:%d] cdc page addr:0x%pK, page0:%#x, page1:%#x, page2:%#x !\n",
		__FUNCTION__, __LINE__, (void *)(unsigned long)(slimbus_dev->page_sel_addr), page0, page1, page2);
}

uint32_t slimbus_trackstate_get(void)
{
	uint32_t trackstate = 0;
	uint32_t track;

	if (!pdata) {
		pr_err("pdata is null\n");
		return SLIMBUS_TRACK_ERROR;
	}

	if(!pdata->track_state) {
		pr_err("[%s:%d] cannot get track state\n", __FUNCTION__, __LINE__);
		return SLIMBUS_TRACK_ERROR;
	}

	for (track = 0; track < pdata->slimbus_track_max; track++) {
		if (pdata->track_state[track])
			trackstate |= (1<<track);
	}

	return trackstate;
}

static bool is_image_scene(uint32_t active_tracks)
{
	if (active_tracks & SLIMBUS_TRACK_IMAGE_LOAD_ON) {
		return true;
	}

	return false;
}

static bool is_play_scene(uint32_t active_tracks)
{
	switch (active_tracks) {
	case SLIMBUS_TRACK_PLAY_ON:
	case SLIMBUS_TRACK_PLAY_ON | SLIMBUS_TRACK_EC_ON:
	case SLIMBUS_TRACK_EC_ON:
		return true;
	default:
		break;
	}

	return false;
}

static bool is_call_only_scene(uint32_t active_tracks)
{
	switch (active_tracks) {
	case SLIMBUS_TRACK_VOICE_UP_ON:
	case SLIMBUS_TRACK_VOICE_DOWN_ON:
	case SLIMBUS_TRACK_VOICE_UP_ON | SLIMBUS_TRACK_VOICE_DOWN_ON:
		return true;
	default:
		break;
	}

	return false;
}

static bool is_call_12288_scene(uint32_t active_tracks)
{
	switch (active_tracks) {
	case SLIMBUS_TRACK_VOICE_UP_ON | SLIMBUS_TRACK_PLAY_ON:
	case SLIMBUS_TRACK_VOICE_DOWN_ON | SLIMBUS_TRACK_PLAY_ON:
	case SLIMBUS_TRACK_VOICE_UP_ON | SLIMBUS_TRACK_VOICE_DOWN_ON | SLIMBUS_TRACK_PLAY_ON:
	case SLIMBUS_TRACK_VOICE_UP_ON | SLIMBUS_TRACK_VOICE_DOWN_ON | SLIMBUS_TRACK_EC_ON:
	case SLIMBUS_TRACK_VOICE_DOWN_ON | SLIMBUS_TRACK_PLAY_ON | SLIMBUS_TRACK_EC_ON:
	case SLIMBUS_TRACK_VOICE_UP_ON | SLIMBUS_TRACK_PLAY_ON | SLIMBUS_TRACK_EC_ON:
	case SLIMBUS_TRACK_VOICE_UP_ON | SLIMBUS_TRACK_VOICE_DOWN_ON | SLIMBUS_TRACK_PLAY_ON | SLIMBUS_TRACK_EC_ON:
		return true;
	default:
		break;
	}

	return false;
}

static bool is_anc_call_scene(uint32_t active_tracks)
{
	switch (active_tracks) {
	case SLIMBUS_TRACK_DEBUG_ON:
	case SLIMBUS_TRACK_DEBUG_ON | SLIMBUS_TRACK_PLAY_ON:
	case SLIMBUS_TRACK_DEBUG_ON | SLIMBUS_TRACK_VOICE_UP_ON:
	case SLIMBUS_TRACK_DEBUG_ON | SLIMBUS_TRACK_VOICE_DOWN_ON:
	case SLIMBUS_TRACK_DEBUG_ON | SLIMBUS_TRACK_VOICE_UP_ON | SLIMBUS_TRACK_VOICE_DOWN_ON:
	case SLIMBUS_TRACK_DEBUG_ON | SLIMBUS_TRACK_VOICE_UP_ON | SLIMBUS_TRACK_PLAY_ON:
	case SLIMBUS_TRACK_DEBUG_ON | SLIMBUS_TRACK_VOICE_DOWN_ON | SLIMBUS_TRACK_PLAY_ON:
	case SLIMBUS_TRACK_DEBUG_ON | SLIMBUS_TRACK_VOICE_UP_ON | SLIMBUS_TRACK_VOICE_DOWN_ON | SLIMBUS_TRACK_PLAY_ON:
		if (track_config_table[SLIMBUS_TRACK_DEBUG].params.channels == 2)
			return true;
		break;
	default:
		break;
	}

	return false;
}

static bool is_enhance_soundtrigger_6144_scene(uint32_t active_tracks)
{
	switch (active_tracks) {
	case SLIMBUS_TRACK_SOUND_TRIGGER_ON:
	case SLIMBUS_TRACK_SOUND_TRIGGER_ON | SLIMBUS_TRACK_PLAY_ON:
	case SLIMBUS_TRACK_SOUND_TRIGGER_ON | SLIMBUS_TRACK_EC_ON:
	case SLIMBUS_TRACK_SOUND_TRIGGER_ON | SLIMBUS_TRACK_PLAY_ON | SLIMBUS_TRACK_EC_ON:
		if (track_config_table[SLIMBUS_TRACK_SOUND_TRIGGER].params.rate == SLIMBUS_SAMPLE_RATE_192K) {
			pr_info("[%s:%d] not st scene", __FUNCTION__, __LINE__);
			return false;
		}
		pr_info("[%s:%d] enhance st scene", __FUNCTION__, __LINE__);
		return true;
	default:
		break;
	}

	return false;
}

static bool is_direct_play_scene(uint32_t active_tracks)
{
	uint32_t tmp = 0;

	switch (active_tracks) {
	case SLIMBUS_TRACK_DIRECT_PLAY_ON:
	case SLIMBUS_TRACK_DIRECT_PLAY_ON | SLIMBUS_TRACK_CAPTURE_ON:
	case SLIMBUS_TRACK_DIRECT_PLAY_ON | SLIMBUS_TRACK_EC_ON:
	case SLIMBUS_TRACK_DIRECT_PLAY_ON | SLIMBUS_TRACK_SOUND_TRIGGER_ON:
	case SLIMBUS_TRACK_DIRECT_PLAY_ON | SLIMBUS_TRACK_PLAY_ON:
	case SLIMBUS_TRACK_DIRECT_PLAY_ON | SLIMBUS_TRACK_CAPTURE_ON | SLIMBUS_TRACK_EC_ON:
	case SLIMBUS_TRACK_DIRECT_PLAY_ON | SLIMBUS_TRACK_CAPTURE_ON | SLIMBUS_TRACK_SOUND_TRIGGER_ON:
	case SLIMBUS_TRACK_DIRECT_PLAY_ON | SLIMBUS_TRACK_EC_ON | SLIMBUS_TRACK_SOUND_TRIGGER_ON:
	case SLIMBUS_TRACK_DIRECT_PLAY_ON | SLIMBUS_TRACK_EC_ON | SLIMBUS_TRACK_PLAY_ON:
	case SLIMBUS_TRACK_DIRECT_PLAY_ON | SLIMBUS_TRACK_CAPTURE_ON | SLIMBUS_TRACK_PLAY_ON:
	case SLIMBUS_TRACK_DIRECT_PLAY_ON | SLIMBUS_TRACK_PLAY_ON | SLIMBUS_TRACK_SOUND_TRIGGER_ON:
	case SLIMBUS_TRACK_DIRECT_PLAY_ON | SLIMBUS_TRACK_CAPTURE_ON | SLIMBUS_TRACK_EC_ON | SLIMBUS_TRACK_PLAY_ON:
	case SLIMBUS_TRACK_DIRECT_PLAY_ON | SLIMBUS_TRACK_PLAY_ON | SLIMBUS_TRACK_EC_ON | SLIMBUS_TRACK_SOUND_TRIGGER_ON:
	case SLIMBUS_TRACK_DIRECT_PLAY_ON | SLIMBUS_TRACK_PLAY_ON | SLIMBUS_TRACK_CAPTURE_ON | SLIMBUS_TRACK_SOUND_TRIGGER_ON:
	case SLIMBUS_TRACK_DIRECT_PLAY_ON | SLIMBUS_TRACK_CAPTURE_ON | SLIMBUS_TRACK_EC_ON | SLIMBUS_TRACK_SOUND_TRIGGER_ON:
	case SLIMBUS_TRACK_DIRECT_PLAY_ON | SLIMBUS_TRACK_PLAY_ON | SLIMBUS_TRACK_CAPTURE_ON | SLIMBUS_TRACK_EC_ON | SLIMBUS_TRACK_SOUND_TRIGGER_ON:
		return true;
	default:
		tmp = (SLIMBUS_TRACK_DEBUG_ON | SLIMBUS_TRACK_DIRECT_PLAY_ON | SLIMBUS_TRACK_FAST_PLAY_ON);
		if ((active_tracks & tmp) == tmp) {
			return false;
		}

		tmp = (SLIMBUS_TRACK_VOICE_DOWN_ON | SLIMBUS_TRACK_VOICE_UP_ON);
		if ((active_tracks & tmp) == tmp) {
			return false;
		}

		tmp = (SLIMBUS_TRACK_DEBUG_ON | SLIMBUS_TRACK_DIRECT_PLAY_ON);
		if ((active_tracks & tmp) == tmp ) {
			return true;
		}
		break;
	}

	return false;
}

static bool is_fast_play_scene(uint32_t active_tracks)
{
	switch (active_tracks) {
	case SLIMBUS_TRACK_FAST_PLAY_ON:
	case SLIMBUS_TRACK_FAST_PLAY_ON | SLIMBUS_TRACK_EC_ON:
		return true;
	default:
		break;
	}

	return false;
}

static bool is_fast_play_record_scene(uint32_t active_tracks)
{
	uint32_t tmp = 0;

	switch (active_tracks) {
	case SLIMBUS_TRACK_FAST_PLAY_ON | SLIMBUS_TRACK_PLAY_ON:
	case SLIMBUS_TRACK_FAST_PLAY_ON | SLIMBUS_TRACK_CAPTURE_ON:
	case SLIMBUS_TRACK_FAST_PLAY_ON | SLIMBUS_TRACK_DIRECT_PLAY_ON:
	case SLIMBUS_TRACK_FAST_PLAY_ON | SLIMBUS_TRACK_EC_ON | SLIMBUS_TRACK_PLAY_ON:
	case SLIMBUS_TRACK_FAST_PLAY_ON | SLIMBUS_TRACK_EC_ON | SLIMBUS_TRACK_CAPTURE_ON:
	case SLIMBUS_TRACK_FAST_PLAY_ON | SLIMBUS_TRACK_PLAY_ON | SLIMBUS_TRACK_CAPTURE_ON:
	case SLIMBUS_TRACK_FAST_PLAY_ON | SLIMBUS_TRACK_EC_ON | SLIMBUS_TRACK_DIRECT_PLAY_ON:
	case SLIMBUS_TRACK_FAST_PLAY_ON | SLIMBUS_TRACK_DIRECT_PLAY_ON | SLIMBUS_TRACK_CAPTURE_ON:
	case SLIMBUS_TRACK_FAST_PLAY_ON | SLIMBUS_TRACK_DIRECT_PLAY_ON | SLIMBUS_TRACK_PLAY_ON:
	case SLIMBUS_TRACK_FAST_PLAY_ON | SLIMBUS_TRACK_EC_ON | SLIMBUS_TRACK_PLAY_ON | SLIMBUS_TRACK_CAPTURE_ON:
	case SLIMBUS_TRACK_FAST_PLAY_ON | SLIMBUS_TRACK_EC_ON | SLIMBUS_TRACK_PLAY_ON | SLIMBUS_TRACK_DIRECT_PLAY_ON:
	case SLIMBUS_TRACK_FAST_PLAY_ON | SLIMBUS_TRACK_CAPTURE_ON | SLIMBUS_TRACK_PLAY_ON | SLIMBUS_TRACK_DIRECT_PLAY_ON:
	case SLIMBUS_TRACK_FAST_PLAY_ON | SLIMBUS_TRACK_EC_ON | SLIMBUS_TRACK_CAPTURE_ON | SLIMBUS_TRACK_DIRECT_PLAY_ON:
	case SLIMBUS_TRACK_FAST_PLAY_ON | SLIMBUS_TRACK_EC_ON | SLIMBUS_TRACK_CAPTURE_ON | SLIMBUS_TRACK_DIRECT_PLAY_ON | SLIMBUS_TRACK_PLAY_ON:
		return true;
	default:
		tmp = (SLIMBUS_TRACK_DEBUG_ON | SLIMBUS_TRACK_FAST_PLAY_ON | SLIMBUS_TRACK_SOUND_TRIGGER_ON);
		if ((active_tracks & tmp) == tmp) {
			return false;
		}

		tmp = (SLIMBUS_TRACK_VOICE_DOWN_ON | SLIMBUS_TRACK_VOICE_UP_ON);
		if ((active_tracks & tmp) == tmp) {
			return false;
		}

		tmp = (SLIMBUS_TRACK_DEBUG_ON | SLIMBUS_TRACK_FAST_PLAY_ON);
		if ((active_tracks & tmp) == tmp) {
			return true;
		}

		break;
	}

	return false;
}

static bool is_fast_play_soundtrigger_scene(uint32_t active_tracks)
{
	uint32_t tmp = 0;

	switch (active_tracks) {
	case SLIMBUS_TRACK_FAST_PLAY_ON | SLIMBUS_TRACK_SOUND_TRIGGER_ON:
	case SLIMBUS_TRACK_FAST_PLAY_ON | SLIMBUS_TRACK_EC_ON | SLIMBUS_TRACK_SOUND_TRIGGER_ON:
	case SLIMBUS_TRACK_FAST_PLAY_ON | SLIMBUS_TRACK_PLAY_ON | SLIMBUS_TRACK_SOUND_TRIGGER_ON:
	case SLIMBUS_TRACK_FAST_PLAY_ON | SLIMBUS_TRACK_DIRECT_PLAY_ON | SLIMBUS_TRACK_SOUND_TRIGGER_ON:
	case SLIMBUS_TRACK_FAST_PLAY_ON | SLIMBUS_TRACK_EC_ON | SLIMBUS_TRACK_PLAY_ON | SLIMBUS_TRACK_SOUND_TRIGGER_ON:
	case SLIMBUS_TRACK_FAST_PLAY_ON | SLIMBUS_TRACK_EC_ON | SLIMBUS_TRACK_DIRECT_PLAY_ON | SLIMBUS_TRACK_SOUND_TRIGGER_ON:
	case SLIMBUS_TRACK_FAST_PLAY_ON | SLIMBUS_TRACK_PLAY_ON | SLIMBUS_TRACK_DIRECT_PLAY_ON | SLIMBUS_TRACK_SOUND_TRIGGER_ON:
	case SLIMBUS_TRACK_FAST_PLAY_ON | SLIMBUS_TRACK_EC_ON | SLIMBUS_TRACK_PLAY_ON | SLIMBUS_TRACK_DIRECT_PLAY_ON | SLIMBUS_TRACK_SOUND_TRIGGER_ON:
		return true;
	default:
		tmp = (SLIMBUS_TRACK_VOICE_DOWN_ON | SLIMBUS_TRACK_VOICE_UP_ON);
		if ((active_tracks & tmp) == tmp) {
			return false;
		}

		tmp = (SLIMBUS_TRACK_FAST_PLAY_ON | SLIMBUS_TRACK_SOUND_TRIGGER_ON);
		if ((active_tracks & tmp) == tmp) {
			return true;
		}
		break;
	}

	return false;
}


void slimbus_trackstate_set(uint32_t track, bool state)
{
	if (!pdata) {
		pr_err("pdata is null\n");
		return;
	}

	if (!pdata->track_state ||(track >= pdata->slimbus_track_max)) {
		pr_err("[%s:%d]track state cannot set\n", __FUNCTION__, __LINE__);
		return;
	}

	pdata->track_state[track] = state;
	return;
}

bool track_state_is_on(uint32_t track)
{
	if (!pdata->track_state ||(track >= pdata->slimbus_track_max)) {
		pr_err("[%s:%d]cannot get track state \n", __FUNCTION__, __LINE__);
		return false;
	}

	return pdata->track_state[track];
}

static int process_direct_and_play_conflict(slimbus_track_type_t track)
{
	int ret = 0;

	if (track_state_is_on(SLIMBUS_TRACK_DIRECT_PLAY) && (track == SLIMBUS_TRACK_AUDIO_PLAY)) {
		pr_info("[%s:%d] direct conflict\n", __FUNCTION__, __LINE__);
		return -EPERM;
	}
	if (track_state_is_on(SLIMBUS_TRACK_AUDIO_PLAY) && (track == SLIMBUS_TRACK_DIRECT_PLAY)) {
		ret = slimbus_drv_track_deactivate(track_config_table[SLIMBUS_TRACK_AUDIO_PLAY].channel_pro,
						track_config_table[SLIMBUS_TRACK_AUDIO_PLAY].params.channels);

		slimbus_trackstate_set(SLIMBUS_TRACK_AUDIO_PLAY, false);
	}

	return ret;
}

static int process_play_and_debug_conflict(slimbus_track_type_t track, int *need_callback)
{
	int ret = 0;

	if ((track_state_is_on(SLIMBUS_TRACK_DIRECT_PLAY) || track_state_is_on(SLIMBUS_TRACK_FAST_PLAY))
		&& (track == SLIMBUS_TRACK_DEBUG)) {
		pr_info("[%s:%d] debug conflict\n", __FUNCTION__, __LINE__);
		return -EPERM;
	}
	if (track_state_is_on(SLIMBUS_TRACK_DEBUG)) {
		*need_callback = 1;
		ret = slimbus_drv_track_deactivate(track_config_table[SLIMBUS_TRACK_DEBUG].channel_pro,
						track_config_table[SLIMBUS_TRACK_DEBUG].params.channels);

		slimbus_trackstate_set(SLIMBUS_TRACK_DEBUG, false);
		track_config_table[SLIMBUS_TRACK_DEBUG].params.channels = 1;
	}

	return ret;
}

static int process_image_and_other_conflict(slimbus_track_type_t track)
{
	int ret = 0;
	int i = 0;

	if (track_state_is_on(SLIMBUS_TRACK_IMAGE_LOAD) && (track != SLIMBUS_TRACK_IMAGE_LOAD)) {
		pr_info("[%s:%d] image conflict\n", __FUNCTION__, __LINE__);
		return -EPERM;
	}
	if (track == SLIMBUS_TRACK_IMAGE_LOAD) {
		for (i = 0; i < SLIMBUS_TRACK_MAX; i++) {
			if (track_state_is_on(i) && (i != track)) {
				pr_info("[%s:%d] image load, deactivate track:%#x \n", __FUNCTION__, __LINE__, i);
				ret += slimbus_drv_track_deactivate(track_config_table[i].channel_pro,
						track_config_table[i].params.channels);
				slimbus_trackstate_set(i, false);
			}
		}
	}

	return ret;
}

static int process_soundtrigger_and_debug_conflict(slimbus_track_type_t track, int *need_callback)
{
	int ret = 0;

	if (track_state_is_on(SLIMBUS_TRACK_SOUND_TRIGGER)
		&& (track == SLIMBUS_TRACK_DEBUG)) {
		pr_info("[%s:%d] st and debug conflict\n", __FUNCTION__, __LINE__);
		return -EPERM;
	}
	if (track_state_is_on(SLIMBUS_TRACK_DEBUG)
		&& (track == SLIMBUS_TRACK_SOUND_TRIGGER)) {
		*need_callback = 1;
		ret = slimbus_drv_track_deactivate(track_config_table[SLIMBUS_TRACK_DEBUG].channel_pro,
				track_config_table[SLIMBUS_TRACK_DEBUG].params.channels);

		slimbus_trackstate_set(SLIMBUS_TRACK_DEBUG, false);
	}

	return ret;
}

static int process_soundtrigger_params_conflict(slimbus_track_type_t track)
{
	bool is_fast_track = ((track_config_table[SLIMBUS_TRACK_SOUND_TRIGGER].params.channels == 2)
		&& (track_config_table[SLIMBUS_TRACK_SOUND_TRIGGER].params.rate == SLIMBUS_SAMPLE_RATE_192K));

	if ((track == SLIMBUS_TRACK_SOUND_TRIGGER) && is_fast_track) {
		track_config_table[track].params.channels = 1;
		pr_info("[%s:%d] soundtrigger conflict\n", __FUNCTION__, __LINE__);
		return -EPERM;
	}

	return 0;
}

static int process_normal_scene_conflict(slimbus_track_type_t track, bool track_enable, int *need_callback)
{
	int ret = 0;

	if (track_state_is_on(SLIMBUS_TRACK_VOICE_UP) && (track == SLIMBUS_TRACK_SOUND_TRIGGER) && track_enable) {
		pr_info("[%s:%d] st conflict\n", __FUNCTION__, __LINE__);
		/*return -EPERM;*///fixme
	}

	if (((track == SLIMBUS_TRACK_DIRECT_PLAY) || (track == SLIMBUS_TRACK_FAST_PLAY)) && track_enable) {
		pr_info("[%s:%d] conflict\n", __FUNCTION__, __LINE__);
		return -EPERM;
	}
	if ((track == SLIMBUS_TRACK_DEBUG) && (track_config_table[SLIMBUS_TRACK_DEBUG].params.channels == 2)) {
		track_config_table[track].params.channels = 1;
		pr_info("[%s:%d] debug conflict\n", __FUNCTION__, __LINE__);
		return -EPERM;
	}
	if (track_state_is_on(SLIMBUS_TRACK_DIRECT_PLAY)) {
		ret = slimbus_drv_track_deactivate(track_config_table[SLIMBUS_TRACK_DIRECT_PLAY].channel_pro,
						track_config_table[SLIMBUS_TRACK_DIRECT_PLAY].params.channels);

		slimbus_trackstate_set(SLIMBUS_TRACK_DIRECT_PLAY, false);
	}
	if (track_state_is_on(SLIMBUS_TRACK_FAST_PLAY)) {
		ret = slimbus_drv_track_deactivate(track_config_table[SLIMBUS_TRACK_FAST_PLAY].channel_pro,
						track_config_table[SLIMBUS_TRACK_FAST_PLAY].params.channels);

		slimbus_trackstate_set(SLIMBUS_TRACK_FAST_PLAY, false);
	}
	if (track_state_is_on(SLIMBUS_TRACK_DEBUG) && (track_config_table[SLIMBUS_TRACK_DEBUG].params.channels == 2)) {
		*need_callback = 1;
		ret = slimbus_drv_track_deactivate(track_config_table[SLIMBUS_TRACK_DEBUG].channel_pro,
						track_config_table[SLIMBUS_TRACK_DEBUG].params.channels);

		slimbus_trackstate_set(SLIMBUS_TRACK_DEBUG, false);
		track_config_table[SLIMBUS_TRACK_DEBUG].params.channels = 1;
	}

	if (process_soundtrigger_and_debug_conflict(track, need_callback)) {
		pr_info("[%s:%d] st and debug conflict\n", __FUNCTION__, __LINE__);
		return -EPERM;
	}

	ret = process_soundtrigger_params_conflict(track);

	return ret;
}

static int process_other_scenes_conflict(slimbus_track_type_t track, int *need_callback)
{
	int ret = 0;

	if (process_direct_and_play_conflict(track)) {
		pr_info("[%s:%d] direct conflict\n", __FUNCTION__, __LINE__);
		return -EPERM;
	}
	if (process_play_and_debug_conflict(track, need_callback)) {
		pr_info("[%s:%d] debug conflict\n", __FUNCTION__, __LINE__);
		return -EPERM;
	}
	if (process_image_and_other_conflict(track)) {
		pr_info("[%s:%d] image conflict\n", __FUNCTION__, __LINE__);
		return -EPERM;
	}

	ret = process_soundtrigger_params_conflict(track);

	return ret;
}

 static int slimbus_check_scenes(
				uint32_t track,
				uint32_t scenes,
				bool track_enable)
{
	unsigned int i = 0;
	int ret = 0;
	int need_callback = 0;

	if (track >= SLIMBUS_TRACK_MAX || scenes >= SLIMBUS_SCENE_CONFIG_MAX) {
		pr_err("param is invalid, track:%d, scenes:%d\n", track, scenes);
		return -EINVAL;
	}

	switch (scenes) {
	case SLIMBUS_SCENE_CONFIG_IMAGE_LOAD:
	case SLIMBUS_SCENE_CONFIG_DIRECT_PLAY:
	case SLIMBUS_SCENE_CONFIG_FAST_PLAY_AND_REC:
	case SLIMBUS_SCENE_CONFIG_FAST_PLAY_AND_ST:
		ret = process_other_scenes_conflict(track, &need_callback);
		break;
	case SLIMBUS_SCENE_CONFIG_NORMAL:
		ret = process_normal_scene_conflict(track, track_enable, &need_callback);
		break;
	default:
		break;
	}

	for (i = 0; i < SLIMBUS_TRACK_MAX; i++) {
		if ((track_state_is_on(i) || (i == SLIMBUS_TRACK_DEBUG)) && track_config_table[i].params.callback && need_callback) {
			track_config_table[i].params.callback(i, (void *)&(track_config_table[i].params));
		}
	}

	return ret;
}


static void fast_select_scenes(
                uint32_t active_tracks,
                slimbus_scene_config_type_t *scene_config_type,
                slimbus_subframe_mode_t *sm,
                slimbus_clock_gear_t *cg)
{
	if (is_fast_play_scene(active_tracks)) {
		*scene_config_type = SLIMBUS_SCENE_CONFIG_FAST_PLAY;
		*cg = (slimbus_clock_gear_t)SLIMBUS_CG_8;
		*sm = (slimbus_subframe_mode_t)SLIMBUS_SM_8_CSW_32_SL;
	} else if (is_fast_play_record_scene(active_tracks)) {
		*scene_config_type = SLIMBUS_SCENE_CONFIG_FAST_PLAY_AND_REC;
		*cg = (slimbus_clock_gear_t)SLIMBUS_CG_10;
		*sm = (slimbus_subframe_mode_t)SLIMBUS_SM_2_CSW_32_SL;
	} else if (is_fast_play_soundtrigger_scene(active_tracks)) {
		*scene_config_type = SLIMBUS_SCENE_CONFIG_FAST_PLAY_AND_ST;
		*cg = (slimbus_clock_gear_t)SLIMBUS_CG_10;
		*sm = (slimbus_subframe_mode_t)SLIMBUS_SM_4_CSW_32_SL;
	} else {
		pr_warn("[%s: %d] not fast scene, %x", __FUNCTION__, __LINE__, active_tracks);
	}

	return;
}

static int slimbus_select_scenes(
				struct slimbus_device_info *dev,
				uint32_t track,
				slimbus_track_param_t *params,
				bool track_enable)
{
	slimbus_scene_config_type_t scene_config_type = SLIMBUS_SCENE_CONFIG_NORMAL;
	slimbus_subframe_mode_t sm = SLIMBUS_SM_1_CSW_32_SL;
	slimbus_clock_gear_t cg = SLIMBUS_CG_10;
	uint32_t active_tracks = 0;
	int ret = 0;
	bool has_fast = false;

	if (track >= SLIMBUS_TRACK_MAX) {
		pr_err("track is invalid, track:%d\n", track);
		return -EINVAL;
	}

	active_tracks = slimbus_trackstate_get();
	if (active_tracks == SLIMBUS_TRACK_ERROR) {
		return -EINVAL;
	}

	if (track_enable) {
		active_tracks |= (1<<track);
	}

	has_fast = ((active_tracks & SLIMBUS_TRACK_FAST_PLAY_ON) == SLIMBUS_TRACK_FAST_PLAY_ON);

	if (is_image_scene(active_tracks)) {
		scene_config_type = SLIMBUS_SCENE_CONFIG_IMAGE_LOAD;
		cg = SLIMBUS_CG_10;
		sm = SLIMBUS_SM_3_CSW_8_SL;
	} else if (is_play_scene(active_tracks)) {
		scene_config_type = SLIMBUS_SCENE_CONFIG_PLAY;
		cg = SLIMBUS_CG_8;
		sm = SLIMBUS_SM_8_CSW_32_SL;
	} else if (is_call_only_scene(active_tracks)) {
		scene_config_type = SLIMBUS_SCENE_CONFIG_CALL;
		cg = SLIMBUS_CG_8;
		sm = SLIMBUS_SM_6_CSW_24_SL;
	} else if (is_call_12288_scene(active_tracks)) {
		scene_config_type = SLIMBUS_SCENE_CONFIG_CALL_12288;
		cg = SLIMBUS_CG_9;
		sm = SLIMBUS_SM_8_CSW_32_SL;
	} else if (is_anc_call_scene(active_tracks)) {
		scene_config_type = SLIMBUS_SCENE_CONFIG_ANC_CALL;
		cg = SLIMBUS_CG_10;
		sm = SLIMBUS_SM_1_CSW_32_SL;
	} else if (is_enhance_soundtrigger_6144_scene(active_tracks)) {
		scene_config_type = SLIMBUS_SCENE_CONFIG_ENHANCE_ST_6144;
		cg = SLIMBUS_CG_8;
		sm = SLIMBUS_SM_4_CSW_32_SL;
	} else if (is_direct_play_scene(active_tracks)) {
		scene_config_type = SLIMBUS_SCENE_CONFIG_DIRECT_PLAY;
		cg = SLIMBUS_CG_10;
		sm = SLIMBUS_SM_2_CSW_32_SL;
	} else if (has_fast) {
		fast_select_scenes(active_tracks, &scene_config_type, &sm, &cg);
	} else {
		scene_config_type = SLIMBUS_SCENE_CONFIG_NORMAL;
		cg = SLIMBUS_CG_10;
		sm = SLIMBUS_SM_1_CSW_32_SL;
	}

	ret = slimbus_check_scenes(track, scene_config_type, track_enable);
	if (ret) {
		pr_info("[%s:%d] ret %d\n", __FUNCTION__, __LINE__, ret);
		return ret;
	}

	if (dev->scene_config_type != scene_config_type) {
		pr_info("[%s:%d] scene changed from %d to %d\n", __FUNCTION__, __LINE__, dev->scene_config_type, scene_config_type);
		dev->cg = cg;
		dev->sm = sm;
		dev->scene_config_type = scene_config_type;
	}

	pdata->dev_ops->slimbus_device_param_update(dev, track, params);

	return 0;
}

static int slimbus_hi64xx_track_soundtrigger_activate(
			uint32_t track,
			bool slimbus_dynamic_freq_enable,
			struct slimbus_device_info *dev,
			slimbus_track_param_t	*params)
{
	int ret = 0;
	slimbus_sound_trigger_params_t  st_params;
	slimbus_track_param_t st_normal_params;
	slimbus_track_config_t *track_config_t = NULL;

	if (SLIMBUS_TRACK_SOUND_TRIGGER != track) {
		pr_err("track %d activate is not soundtriger\n", track);
		return -1;
	}

	memset(&st_params, 0, sizeof(st_params));
	memset(&st_normal_params, 0, sizeof(st_normal_params));

	pdata->dev_ops->slimbus_get_soundtrigger_params(&st_params);
	st_normal_params.channels = st_params.channels;
	st_normal_params.rate = st_params.sample_rate;

	ret = pdata->dev_ops->slimbus_device_param_set(dev, st_params.track_type,
				 &st_normal_params);
	ret += pdata->dev_ops->slimbus_device_param_set(dev, track,
				params);
	if (ret) {
		pr_err("slimbus device param set failed, ret = %d\n", ret);
		return ret;
	}

	/*  request soc slimbus clk to 24.576M */
	slimbus_freq_request();
	track_config_t = dev->slimbus_64xx_para->track_config_table;

	if (slimbus_dynamic_freq_enable) {
		ret = slimbus_drv_track_update(dev->cg, dev->sm, track, dev,
			track_config_t[track].params.channels, track_config_t[track].channel_pro);
		ret += slimbus_drv_track_update(dev->cg, dev->sm, st_params.track_type, dev,
			SLIMBUS_VOICE_UP_SOUNDTRIGGER, track_config_t[st_params.track_type].channel_pro);
	} else {
		ret = slimbus_drv_track_activate(track_config_t[track].channel_pro, track_config_t[track].params.channels);
		ret += slimbus_drv_track_activate(track_config_t[st_params.track_type].channel_pro, SLIMBUS_VOICE_UP_SOUNDTRIGGER);
	}

	return ret;

}

static int slimbus_check_pm(uint32_t track)
{
	int ret = 0;

	if (pdata->pm_runtime_support) {
		if (pdata->track_state[track]) {
			pr_info("[%s:%d] track:%d has been configured \n", __FUNCTION__, __LINE__, track);
		} else {
			ret = pm_runtime_get_sync(pdata->dev);
			if (ret < 0) {
				pr_err("[%s:%d] pm resume error, track:%d, ret:%d\n", __FUNCTION__, __LINE__, track, ret);
				BUG_ON(true);
				return ret;
			}
			if (!slimbus_trackstate_get()) {
				ret = slimbus_switch_framer(pdata->device_type, SLIMBUS_FRAMER_CODEC);
				if (ret) {
					pr_err("[%s:%d] slimbus switch framer to codec failed!\n", __FUNCTION__, __LINE__);
				}
			}
		}
	}

	return ret;
}

static void slimbus_hi64xx_check_st_conflict(uint32_t track, slimbus_track_param_t *params)
{
	int res = 0;
	bool is_fast_track_on = (track_state_is_on(SLIMBUS_TRACK_SOUND_TRIGGER)
		&& (track_config_table[SLIMBUS_TRACK_SOUND_TRIGGER].params.rate == SLIMBUS_SAMPLE_RATE_192K));

	if ((SLIMBUS_TRACK_SOUND_TRIGGER == track)
		&& params
		&& (params->rate == SLIMBUS_SAMPLE_RATE_16K)
		&& is_fast_track_on) {
		pr_info("st conflict, so stop codec st\n");

		res = slimbus_drv_track_deactivate(track_config_table[SLIMBUS_TRACK_SOUND_TRIGGER].channel_pro,
						track_config_table[SLIMBUS_TRACK_SOUND_TRIGGER].params.channels);

		slimbus_trackstate_set(SLIMBUS_TRACK_SOUND_TRIGGER, false);
	}

	if (res != 0) {
		pr_info("start soc st ,stop codec st fail\n");
	}
}

static bool slimbus_hi64xx_track_is_fast_soundtrigger(uint32_t track)
{
	if (SLIMBUS_TRACK_SOUND_TRIGGER != track) {
		return false;
	}

	return ((SLIMBUS_TRACK_SOUND_TRIGGER == track)
		&& (track_config_table[track].params.rate == SLIMBUS_SAMPLE_RATE_192K));
}

static bool slimbus_track_params_is_valid(slimbus_device_type_t dev_type, uint32_t track)
{
	struct slimbus_device_info *dev;

	if (!pdata) {
		pr_err("pdata is null\n");
		return false;
	}

	if (!pdata->track_state || (track >= pdata->slimbus_track_max)) {
		pr_err("params error, track %d\n", track);
		return false;
	}

	if (dev_type >= SLIMBUS_DEVICE_NUM) {
		pr_err("params error, dev_type %dn", dev_type);
		return false;
	}

	dev = slimbus_devices[dev_type];
	if (NULL == dev) {
		pr_err("slimbus havn't been init\n");
		return false;
	}

	if (NULL == dev->slimbus_64xx_para) {
		pr_err("slimbus para havn't been init\n");
		return false;
	}

	if (NULL == dev->slimbus_64xx_para->track_config_table) {
		pr_err("slimbus track config havn't been init\n");
		return false;
	}

	return true;
}

int slimbus_track_activate(
				slimbus_device_type_t dev_type,
				uint32_t track,
				slimbus_track_param_t *params)
{
	int ret = 0;
	struct slimbus_device_info *dev = NULL;
	bool is_fast_soundtrigger = false;
	slimbus_track_config_t *track_config_t = NULL;

	if (!slimbus_track_params_is_valid(dev_type, track)) {
		pr_err("params error, dev_type %d, track %d\n", dev_type, track);
		return -1;
	}

	dev = slimbus_devices[dev_type];
	track_config_t = dev->slimbus_64xx_para->track_config_table;

	mutex_lock(&dev->track_mutex);

	ret = slimbus_check_pm(track);
	if (ret < 0) {
		mutex_unlock(&dev->track_mutex);
		return ret;
	}

	if (pdata->dev_ops->slimbus_check_st_conflict)
		pdata->dev_ops->slimbus_check_st_conflict(track, params);

	if (params) {
		track_config_t[track].params.channels = params->channels;
		track_config_t[track].params.rate = params->rate;
		track_config_t[track].params.callback = params->callback;
	}

	if (pdata->slimbus_dynamic_freq_enable) {
		ret = pdata->dev_ops->slimbus_select_scenes(dev, track, params, true);
	} else {
		ret = pdata->dev_ops->slimbus_check_scenes(track, dev->scene_config_type, true);
	}
	if (ret) {
		pr_err("slimbus activate track %d fail \n", track);
		if (pdata->pm_runtime_support && !pdata->track_state[track]) {
			pm_runtime_mark_last_busy(pdata->dev);
			pm_runtime_put_autosuspend(pdata->dev);
		}
		mutex_unlock(&dev->track_mutex);
		return ret;
	}

	is_fast_soundtrigger = pdata->dev_ops->slimbus_track_is_fast_soundtrigger(track);
	if (is_fast_soundtrigger) {
		ret = pdata->dev_ops->slimbus_track_soundtrigger_activate(track,
				pdata->slimbus_dynamic_freq_enable, dev, params);
	} else {
		ret = pdata->dev_ops->slimbus_device_param_set(dev, track, params);
		if (pdata->slimbus_dynamic_freq_enable) {
			ret += slimbus_drv_track_update(dev->cg, dev->sm, track, dev,
				track_config_t[track].params.channels,
				track_config_t[track].channel_pro);
		} else {
			ret += slimbus_drv_track_activate(track_config_t[track].channel_pro,
				track_config_t[track].params.channels);
		}
	}

	if (!ret) {
		pdata->track_state[track] = true;
	}

	mutex_unlock(&dev->track_mutex);

	return ret;
}

static int slimbus_hi64xx_track_soundtrigger_deactivate(uint32_t track)
{
	int ret = 0;
	slimbus_sound_trigger_params_t  st_params;

	if (SLIMBUS_TRACK_SOUND_TRIGGER != track) {
		pr_err("[%s:%d] track %d deactive is not soundtriger\n", __FUNCTION__, __LINE__, track);
		return -1;
	}

	memset(&st_params, 0, sizeof(st_params));
	pdata->dev_ops->slimbus_get_soundtrigger_params(&st_params);

	/*  release soc slimbus clk to 21.777M */
	slimbus_freq_release();

	ret = slimbus_drv_track_deactivate(track_config_table[track].channel_pro, track_config_table[track].params.channels);
	ret += slimbus_drv_track_deactivate(track_config_table[st_params.track_type].channel_pro, SLIMBUS_VOICE_UP_SOUNDTRIGGER);

	return ret;
}

int slimbus_track_deactivate(
				slimbus_device_type_t dev_type,
				uint32_t track,
				slimbus_track_param_t *params)
{
	int ret = 0;
	struct slimbus_device_info *dev = NULL;
	bool is_fast_soundtrigger = false;
	slimbus_track_config_t *track_config_t = NULL;

	if (!slimbus_track_params_is_valid(dev_type, track)) {
		pr_err("params error, dev_type %d, track %d\n", dev_type, track);
		return -1;
	}

	dev = slimbus_devices[dev_type];
	track_config_t = dev->slimbus_64xx_para->track_config_table;

	mutex_lock(&dev->track_mutex);

	if (pdata->pm_runtime_support) {
		if (!pdata->track_state[track]) {
			pr_err("[%s:%d] track:%d has been removed \n", __FUNCTION__, __LINE__, track);
			mutex_unlock(&dev->track_mutex);
			return 0;
		}
	}

	if (params) {
		track_config_t[track].params.channels = params->channels;
		track_config_t[track].params.rate = params->rate;
		track_config_t[track].params.callback = params->callback;
	}

	is_fast_soundtrigger = pdata->dev_ops->slimbus_track_is_fast_soundtrigger(track);
	if (is_fast_soundtrigger) {
		ret = pdata->dev_ops->slimbus_track_soundtrigger_deactivate(track);
	} else {
		ret = slimbus_drv_track_deactivate(track_config_t[track].channel_pro,
				track_config_t[track].params.channels);
	}

	if (!ret) {
		pdata->track_state[track] = false;
	} else {
		pr_err("[%s:%d] fail track %d, ret %d\n", __FUNCTION__, __LINE__, track, ret);
	}

	if (pdata->slimbus_dynamic_freq_enable) {
		ret = pdata->dev_ops->slimbus_select_scenes(dev, track, NULL, false);
		ret = slimbus_drv_track_update(dev->cg, dev->sm, track, dev, 0, NULL);
	}

	pr_info("[%s:%d] track:%d trackstate:0x%x portstate:0x%x pm_usage_count:0x%x\n", __FUNCTION__, __LINE__,
		track, slimbus_trackstate_get(), slimbus_port_state_get(pdata->base_addr), atomic_read(&(pdata->dev->power.usage_count)));

	if (pdata->pm_runtime_support) {
		if (!slimbus_trackstate_get()) {
			ret = slimbus_switch_framer(pdata->device_type, SLIMBUS_FRAMER_SOC);
			if (ret) {
				pr_err("[%s:%d] slimbus switch framer to soc failed!\n", __FUNCTION__, __LINE__);
			}
		}

		pm_runtime_mark_last_busy(pdata->dev);
		pm_runtime_put_autosuspend(pdata->dev);
	}

	mutex_unlock(&dev->track_mutex);

	return ret;
}

static bool slimbus_switch_framer_params_check(slimbus_device_type_t	dev_type,
	slimbus_framer_type_t	framer_type)
{
	if (!pdata) {
		pr_err("pdata is null\n");
		return false;
	}

	if (dev_type >= SLIMBUS_DEVICE_NUM || framer_type >= SLIMBUS_FRAMER_NUM) {
		pr_err("param is invalid, dev_type:%d, framer_type:%d\n", dev_type, framer_type);
		return false;
	}

	return true;
}

int slimbus_switch_framer(slimbus_device_type_t	dev_type,
				slimbus_framer_type_t	framer_type)
{
	int ret = -1;
	uint8_t la;
	struct slimbus_device_info *dev = NULL;
	slimbus_bus_config_t   *bus_cfg = NULL;

	if (!slimbus_switch_framer_params_check(dev_type, framer_type)) {
		pr_err("params check fail\n");
		return -EINVAL;
	}

	if (NULL == slimbus_devices[dev_type]) {
		pr_err("slimbus havn't been init\n");
		return -EINVAL;
	}

	dev = slimbus_devices[dev_type];

	if (framer_type == SLIMBUS_FRAMER_CODEC) {
		/*  modify soc slimbus clk to 24.576M */
		slimbus_freq_request();
		bus_cfg = &bus_config[SLIMBUS_BUS_CONFIG_SWITCH_FRAMER];
	} else if (framer_type == SLIMBUS_FRAMER_SOC) {
		bus_cfg = &bus_config[SLIMBUS_BUS_CONFIG_NORMAL];
		/*  modify soc slimbus clk to low to avoid signal interference to GPS */
		slimbus_freq_release();
	} else {
		pr_err("[%s:%d] invalid framer_type:%#x \n", __FUNCTION__, __LINE__, framer_type);
		return -EINVAL;
	}

	la = slimbus_drv_get_framerla(framer_type);
	if (la == 0) {
		pr_err("[%s:%d] invalid la:%d framer_type:%#x \n", __FUNCTION__, __LINE__, la, framer_type);
		return -EINVAL;
	}

	if (pdata->switch_framer_disable)
		return 0;

	if (dev->rf == SLIMBUS_RF_6144) {
		if (framer_type == SLIMBUS_FRAMER_CODEC) {
			ret = slimbus_drv_switch_framer(la, 4, 18, bus_cfg);
		} else if (framer_type == SLIMBUS_FRAMER_SOC) {
			ret = slimbus_drv_switch_framer(la, 17, 3, bus_cfg);
		}
	} else if (dev->rf == SLIMBUS_RF_24576) {
		ret = slimbus_drv_switch_framer(la, 4, 3, bus_cfg);
	}

	if (EOK == ret) {
		pdata->framerstate =  framer_type;
	} else {
		pr_err("slimbus_switch_framer faild, ret = %d, framer_type = %d!\n", ret, framer_type);
	}

	return ret;
}

int slimbus_pause_clock(
				slimbus_device_type_t	dev_type,
				slimbus_restart_time_t	newrestarttime)
{
	int ret = 0;

	if (dev_type >= SLIMBUS_DEVICE_NUM) {
		pr_err("[%s:%d] device type is invalid, dev_type:%d\n", __FUNCTION__, __LINE__, dev_type);
		return -EINVAL;
	}

	if (NULL == slimbus_devices[dev_type]) {
		pr_err("slimbus havn't been init\n");
		return -1;
	}

	ret = slimbus_drv_pause_clock(newrestarttime);

	return ret;
}

int slimbus_track_recover(void)
{
	uint32_t track_type;
	int ret = 0;

	if (!pdata->track_state) {
		pr_err("[%s:%d] cannot get track state \n", __FUNCTION__, __LINE__);
		return -1;
	}

	for (track_type = 0; track_type < pdata->slimbus_track_max; track_type++) {
		if (pdata->track_state[track_type]) {
			pr_info("[%s:%d] recover track:%#x \n", __FUNCTION__, __LINE__, track_type);
			ret += slimbus_track_activate(pdata->device_type, track_type, NULL);
		}
	}

	return ret;
}

slimbus_framer_type_t slimbus_debug_get_framer(void)
{
	if (!pdata)
	{
		pr_err("[%s:%d] cannot get framer\n", __FUNCTION__, __LINE__);
		return SLIMBUS_FRAMER_SOC;
	}
	return pdata->framerstate;
}

slimbus_device_type_t slimbus_debug_get_device_type(void)
{
	if (!pdata)
	{
		pr_err("[%s:%d] cannot get device type\n", __FUNCTION__, __LINE__);
		return SLIMBUS_DEVICE_NUM;
	}
	return pdata->device_type;
}

int slimbus_bus_configure(slimbus_bus_config_type_t type)
{
	int ret = 0;
	int pm_ret = 0;

	if (!pdata) {
		pr_err("pdata is null\n");
		return -EINVAL;
	}

	if (type >= SLIMBUS_BUS_CONFIG_MAX) {
		pr_err("[%s:%d] type is invalid, type:%d\n", __FUNCTION__, __LINE__, type);
		return -EINVAL;
	}

	if (pdata->pm_runtime_support) {
		pm_ret = pm_runtime_get_sync(pdata->dev);
		if (pm_ret < 0) {
			pr_err("[%s:%d] pm resume error, type:%d pm_ret:%d\n", __FUNCTION__, __LINE__, type, pm_ret);
			BUG_ON(true);
			return pm_ret;
		}
	}

	ret = slimbus_drv_bus_configure(&bus_config[type]);

	if (pdata->pm_runtime_support) {
		pm_runtime_mark_last_busy(pdata->dev);
		pm_runtime_put_autosuspend(pdata->dev);
	}

	return ret;
}

static void slimbus_ioparam_get(slimbus_device_type_t device_type)
{
	uint32_t   slimbusclk_drv = 0;
	uint32_t   slimbusdata_drv = 0;
	uint32_t   slimbusclk_offset = 0;
	uint32_t   slimbusdata_offset = 0;
	uint32_t   slimbusclk_cfg_offset = 0;
	uint32_t   slimbusdata_cfg_offset = 0;

	if (!of_property_read_u32(pdata->dev->of_node, "slimbusclk_io_driver", &slimbusclk_drv)) {
		slimbus_devices[device_type]->slimbusclk_drv = slimbusclk_drv;
	}

	if (!of_property_read_u32(pdata->dev->of_node, "slimbusdata_io_driver", &slimbusdata_drv)) {
		slimbus_devices[device_type]->slimbusdata_drv = slimbusdata_drv;
	}

	if (!of_property_read_u32(pdata->dev->of_node, "slimbusclk_offset", &slimbusclk_offset)) {
		slimbus_devices[device_type]->slimbusclk_offset = slimbusclk_offset;
	}

	if (!of_property_read_u32(pdata->dev->of_node, "slimbusdata_offset", &slimbusdata_offset)) {
		slimbus_devices[device_type]->slimbusdata_offset = slimbusdata_offset;
	}

	if (!of_property_read_u32(pdata->dev->of_node, "slimbusclk_cfg_offset", &slimbusclk_cfg_offset)) {
		slimbus_devices[device_type]->slimbusclk_cfg_offset = slimbusclk_cfg_offset;
	}

	if (!of_property_read_u32(pdata->dev->of_node, "slimbusdata_cfg_offset", &slimbusdata_cfg_offset)) {
		slimbus_devices[device_type]->slimbusdata_cfg_offset = slimbusdata_cfg_offset;
	}
}

static int slimbus_init_platform_params(const char *platformtype, slimbus_device_type_t device_type, platform_type_t *platform_type)
{
	int ret = 0;

	slimbus_devices[device_type]->rf = SLIMBUS_RF_6144;
	slimbus_devices[device_type]->slimbusclk_drv = 0xA8;
	slimbus_devices[device_type]->slimbusdata_drv = 0xA3;
	slimbus_devices[device_type]->slimbusclk_offset = IOC_SYS_IOMG_011;
	slimbus_devices[device_type]->slimbusdata_offset = IOC_SYS_IOMG_012;
	slimbus_devices[device_type]->slimbusclk_cfg_offset = IOC_SYS_IOCG_013;
	slimbus_devices[device_type]->slimbusdata_cfg_offset = IOC_SYS_IOCG_014;
	*platform_type = PLATFORM_PHONE;

	if(NULL == platformtype){
		pr_info("[%s:%d] platform not define! default:ASIC!\n", __FUNCTION__, __LINE__);
		slimbus_devices[device_type]->rf = SLIMBUS_RF_24576;
		bus_config[SLIMBUS_BUS_CONFIG_NORMAL].sm = SLIMBUS_SM_8_CSW_32_SL;
		bus_config[SLIMBUS_BUS_CONFIG_SWITCH_FRAMER].sm = SLIMBUS_SM_8_CSW_32_SL;
		bus_config[SLIMBUS_BUS_CONFIG_SWITCH_FRAMER].cg = SLIMBUS_CG_10;
		slimbus_devices[device_type]->scene_config_type = SLIMBUS_SCENE_CONFIG_NORMAL;
	} else {
		if (!strncmp(platformtype, "ASIC", 4)) {
			slimbus_devices[device_type]->rf = SLIMBUS_RF_24576;
			bus_config[SLIMBUS_BUS_CONFIG_NORMAL].sm = SLIMBUS_SM_8_CSW_32_SL;
			bus_config[SLIMBUS_BUS_CONFIG_SWITCH_FRAMER].sm = SLIMBUS_SM_8_CSW_32_SL;
			bus_config[SLIMBUS_BUS_CONFIG_SWITCH_FRAMER].cg = SLIMBUS_CG_10;
			slimbus_devices[device_type]->scene_config_type = SLIMBUS_SCENE_CONFIG_NORMAL;
		} else if (!strncmp(platformtype, "UDP", 3)) {
			slimbus_devices[device_type]->rf = SLIMBUS_RF_24576;
			slimbus_devices[device_type]->slimbusclk_drv = 0xC0;
			slimbus_devices[device_type]->slimbusdata_drv = 0xC3;
			bus_config[SLIMBUS_BUS_CONFIG_NORMAL].sm = SLIMBUS_SM_8_CSW_32_SL;
			bus_config[SLIMBUS_BUS_CONFIG_SWITCH_FRAMER].sm = SLIMBUS_SM_8_CSW_32_SL;
			bus_config[SLIMBUS_BUS_CONFIG_SWITCH_FRAMER].cg = SLIMBUS_CG_10;
			slimbus_devices[device_type]->scene_config_type = SLIMBUS_SCENE_CONFIG_NORMAL;
			*platform_type = PLATFORM_UDP;
		} else if (!strncmp(platformtype, "FPGA", 4)) {
			slimbus_devices[device_type]->rf = SLIMBUS_RF_6144;
			bus_config[SLIMBUS_BUS_CONFIG_NORMAL].sm = SLIMBUS_SM_4_CSW_32_SL;
			bus_config[SLIMBUS_BUS_CONFIG_SWITCH_FRAMER].sm = SLIMBUS_SM_4_CSW_32_SL;
			bus_config[SLIMBUS_BUS_CONFIG_SWITCH_FRAMER].cg = SLIMBUS_CG_8;
			if (SLIMBUS_DEVICE_HI6403 == device_type) {
				slimbus_devices[device_type]->scene_config_type = SLIMBUS_SCENE_CONFIG_6144_FPGA;
			}
			*platform_type = PLATFORM_FPGA;
		} else {
			pr_err("platform type define err, platformtype %s!\n", platformtype);
			ret = -EINVAL;
			return ret;
		}
	}

	slimbus_ioparam_get(device_type);

	if ((SLIMBUS_DEVICE_HI6403 == device_type) && (*platform_type != PLATFORM_FPGA)) {
		bus_config[SLIMBUS_BUS_CONFIG_NORMAL].sm = SLIMBUS_SM_1_CSW_32_SL;
		bus_config[SLIMBUS_BUS_CONFIG_SWITCH_FRAMER].sm = SLIMBUS_SM_1_CSW_32_SL;
		slimbus_devices[device_type]->cg = SLIMBUS_CG_10;
		slimbus_devices[device_type]->sm = SLIMBUS_SM_1_CSW_32_SL;
	}

	pr_info("[%s:%d] platform type:%s device_type:%d slimbusclk:0x%x, slimbusdata:0x%x, slimbusclk_cfg:0x%x slimbusdata_cfg:0x%x\n",
		__FUNCTION__, __LINE__, platformtype, device_type,
		slimbus_devices[device_type]->slimbusclk_offset,
		slimbus_devices[device_type]->slimbusdata_offset,
		slimbus_devices[device_type]->slimbusclk_cfg_offset,
		slimbus_devices[device_type]->slimbusdata_cfg_offset);

	if (pdata)
		pdata->dev_ops->slimbus_device_param_init(slimbus_devices[device_type]);

	return ret;
}

static void slimbus_hi64xx_register(slimbus_device_ops_t *dev_ops, struct slimbus_private_data *pd)
{
	dev_ops->release_slimbus_device = release_hi64xx_slimbus_device;
	dev_ops->slimbus_track_soundtrigger_activate = slimbus_hi64xx_track_soundtrigger_activate;
	dev_ops->slimbus_track_soundtrigger_deactivate = slimbus_hi64xx_track_soundtrigger_deactivate;
	dev_ops->slimbus_track_is_fast_soundtrigger = slimbus_hi64xx_track_is_fast_soundtrigger;
	dev_ops->slimbus_check_st_conflict = slimbus_hi64xx_check_st_conflict;
	dev_ops->slimbus_check_scenes = slimbus_check_scenes;
	dev_ops->slimbus_select_scenes = slimbus_select_scenes;

	pd->track_config_table = track_config_table;
	pd->slimbus_track_max = SLIMBUS_TRACK_MAX;

	if (SLIMBUS_DEVICE_HI6402 == pd->device_type) {
		dev_ops->create_slimbus_device = create_hi6402_slimbus_device;
		dev_ops->release_slimbus_device = release_hi6402_slimbus_device;
		dev_ops->slimbus_device_param_init = slimbus_hi6402_param_init;
		dev_ops->slimbus_device_param_set = slimbus_hi6402_param_set;
		dev_ops->slimbus_device_param_update = NULL;
		dev_ops->slimbus_get_soundtrigger_params = slimbus_hi6402_get_st_params;
	} else if (SLIMBUS_DEVICE_HI6403 == pd->device_type) {
		dev_ops->create_slimbus_device = create_hi6403_slimbus_device;
		dev_ops->slimbus_device_param_set = slimbus_hi6403_param_set;
		dev_ops->slimbus_device_param_init = slimbus_hi6403_param_init;
		dev_ops->slimbus_device_param_update = slimbus_hi6403_param_update;
		dev_ops->slimbus_get_soundtrigger_params = slimbus_hi6403_get_st_params;
	}
	return;
}

static int slimbus_clk_init(struct platform_device *pdev, struct slimbus_private_data *pd)
{
	int ret = 0;
	struct device *dev = &pdev->dev;
	struct resource *resource = NULL;
	uint32_t   clk_asp_subsys = 0;

	/* get pmu audio clk */
	pd->pmu_audio_clk = devm_clk_get(dev, "clk_pmuaudioclk");
	if (IS_ERR_OR_NULL(pd->pmu_audio_clk)) {
		dev_err(dev, "devm_clk_get: pmu_audio_clk not found!\n");
		return -EFAULT;
	}

	ret = clk_prepare_enable(pd->pmu_audio_clk);
	if (ret) {
		dev_err(dev, "pmu_audio_clk :clk prepare enable failed !\n");
		goto get_pmu_audio_clk_err;
	}
	mdelay(1);

	pd->asp_subsys_clk = devm_clk_get(dev, "clk_asp_subsys");
	if (IS_ERR_OR_NULL(pd->asp_subsys_clk)) {
		dev_err(dev, "devm_clk_get: clk_asp_subsys not found!\n");
		goto  pmu_audio_clk_enable_err;
	}

	ret = clk_prepare_enable(pd->asp_subsys_clk);
	if (ret) {
		dev_err(dev, "asp_subsys_clk :clk prepare enable failed !\n");
		goto asp_subsys_clk_clk_err;
	}

	/* SLIMbus base address */
	resource = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	if (!resource) {
		dev_err(dev, "get IORESOURCE_MEM failed\n");
		goto asp_subsys_clk_enable_err;
	}

	pd->base_addr = ioremap(resource->start, resource_size(resource));
	if (!pd->base_addr) {
		dev_err(dev, "remap base address %pK failed\n", (void*)resource->start);
		goto asp_subsys_clk_enable_err;
	}

	resource = platform_get_resource(pdev, IORESOURCE_MEM, 1);
	if (!resource) {
		dev_err(dev, "get IORESOURCE_MEM failed\n");
		goto get_aspres_err;
	}

	pd->asp_reg_base_addr = ioremap(resource->start, resource_size(resource));
	if (!pd->asp_reg_base_addr) {
		dev_err(dev, "remap base address %pK failed\n", (void*)resource->start);
		goto get_aspres_err;
	}

	ret = of_property_read_u32(pdev->dev.of_node, "clk_asp_subsys", &clk_asp_subsys);
	if (ret) {
		pr_info("clk_asp_subsyss not define! use default:480000M\n");
		clk_asp_subsys = 480000;
	}

	slimbus_utils_init(pd->asp_reg_base_addr, clk_asp_subsys);

	return 0;
get_aspres_err:
	iounmap(pd->base_addr);
asp_subsys_clk_enable_err:
	clk_disable_unprepare(pd->asp_subsys_clk);
asp_subsys_clk_clk_err:
pmu_audio_clk_enable_err:
	clk_disable_unprepare(pd->pmu_audio_clk);
get_pmu_audio_clk_err:

	return -EFAULT;
}

static int slimbus_asp_power_init(struct platform_device *pdev, struct resource *resource, struct slimbus_private_data *pd)
{
	int ret = 0;
	struct device *dev = &pdev->dev;

	if (!resource) {
		pr_err("get sctrl base addr failed\n");
	} else {
		pd->sctrl_base_addr = ioremap(resource->start, resource_size(resource));
		if (!pd->sctrl_base_addr) {
			pr_err("remap base address %pK failed\n", (void*)resource->start);
			return -ENOMEM;
		}

		if (of_property_read_u32(pdev->dev.of_node, "asp_power_state_offset", &(pd->asp_power_state_offset))) {
			pr_err("of_property_read_u32 return error!\n");
			return -EFAULT;
		}

		if (of_property_read_u32(pdev->dev.of_node, "asp_clk_state_offset", &(pd->asp_clk_state_offset))) {
			pr_err("of_property_read_u32 return error!\n");
			return -EFAULT;
		}

		pr_info("[%s:%d] sctrl base addr:0x%pK, virtaddr:0x%pK, power-offset:0x%x clk-offset:0x%x\n",
			__FUNCTION__, __LINE__, (void*)resource->start, pd->sctrl_base_addr, pd->asp_power_state_offset, pd->asp_clk_state_offset);
	}

	pr_info("[%s:%d] virtual address slimbus:%pK, asp:%pK!\n", __FUNCTION__, __LINE__,pd->base_addr, pd->asp_reg_base_addr);

	/* SLIMbus irq */
	pd->irq = platform_get_irq_byname(pdev, "asp_irq_slimbus");
	if (pd->irq < 0) {
		pr_err("get irq failed\n");
		return -EFAULT;
	}

	pd->regu_asp = devm_regulator_get(dev, "slimbus-reg");
	if (IS_ERR(pd->regu_asp)) {
		pr_err("couldn't get regulators !\n");
		return -EFAULT;
	}

	ret = regulator_enable(pd->regu_asp);
	if (ret) {
		pr_err("couldn't enable regulators %d\n", ret);
		return -EFAULT;
	}

	return ret;
}

static int slimbus_pinctrl_init(struct platform_device *pdev, struct slimbus_private_data *pd)
{
	int ret = 0;
	int get_err = -1;
	int ops_err = -2;
	const size_t max_size_codectype = 14;
	slimbus_device_ops_t *dev_ops = NULL;
	const char *codectype = NULL;
	struct device *dev = &pdev->dev;

	/* ssi&slimbus iomux config */
	pd->pctrl = pinctrl_get(dev);
	if (IS_ERR(pd->pctrl)) {
		dev_err(dev, "could not get pinctrl\n");
		return get_err;
	}
	pd->pin_default = pinctrl_lookup_state(pd->pctrl, PINCTRL_STATE_DEFAULT);
	if (IS_ERR(pd->pin_default)) {
		dev_err(dev, "%s : could not get defstate (%li)\n", __FUNCTION__ , PTR_ERR(pd->pin_default));
		return ops_err;
	}
	pd->pin_idle = pinctrl_lookup_state(pd->pctrl, PINCTRL_STATE_IDLE);
	if (IS_ERR(pd->pin_idle)) {
		dev_err(dev, "%s : could not get defstate (%li)\n", __FUNCTION__ , PTR_ERR(pd->pin_idle));
		return ops_err;
	}

	ret = pinctrl_select_state(pd->pctrl, pd->pin_default);
	if (ret) {
		dev_err(dev, "%s : could not set pins to default state\n", __FUNCTION__);
		return ops_err;
	}

	dev_ops = devm_kzalloc(dev, sizeof(*dev_ops), GFP_KERNEL);
	if(!dev_ops){
		pr_err("%s : kzalloc error!\n", __FUNCTION__);
		return ops_err;
	}

	pd->dev_ops = dev_ops;

	ret = of_property_read_string(pdev->dev.of_node, "codec-type", &codectype);

	if (ret == 0 && !strncmp(codectype, "slimbus-6403cs", max_size_codectype)) {
		pd->device_type = SLIMBUS_DEVICE_HI6403;
	}
	else {
		pd->device_type = SLIMBUS_DEVICE_HI6402;
	}

	slimbus_hi64xx_register(dev_ops, pd);
	pd->track_state = devm_kzalloc(dev, sizeof(bool) * pd->slimbus_track_max, GFP_KERNEL);
	if(!pd->track_state){
		pr_err("%s : kzalloc error!\n", __FUNCTION__);
		return ops_err;
	}

	ret = dev_ops->create_slimbus_device(&slimbus_devices[pd->device_type]);
	if (ret) {
		dev_err(dev, "slimbus device create failed! \n");
		return ops_err;
	}

	return ret;
}

static void slimbus_pm_init(struct platform_device *pdev, struct slimbus_private_data *pd)
{
	if (of_property_read_bool(pdev->dev.of_node, "pm_runtime_support"))
		pd->pm_runtime_support = true;

	pr_info("[%s:%d] pm_runtime_suppport:%d!\n", __FUNCTION__, __LINE__,pd->pm_runtime_support);

	if (pd->pm_runtime_support) {
		pm_runtime_use_autosuspend(&pdev->dev);
		pm_runtime_set_autosuspend_delay(&pdev->dev, 200); /* 200ms */
		pm_runtime_set_active(&pdev->dev);
		pm_suspend_ignore_children(&pdev->dev, true);
		pm_runtime_enable(&pdev->dev);
	}

	return;
}

static int slimbus_probe(struct platform_device *pdev)
{
	struct slimbus_private_data *pd;
	struct device	*dev = &pdev->dev;
	struct resource *resource;
	const char *platformtype = NULL;
	int   ret          = 0;
	const char *property_value = NULL;
	int get_err = -1;
	int ops_err = -2;

	pd = devm_kzalloc(dev, sizeof(struct slimbus_private_data), GFP_KERNEL);
	if (!pd) {
		dev_err(dev, "not enough memory for slimbus_private_data\n");
		return -ENOMEM;
	}

	ret = slimbus_clk_init(pdev, pd);
	if (ret != 0) {
		pr_info("[%s:%d] clk init fail!\n", __FUNCTION__, __LINE__);
		return ret;
	}

	/* get asp power state address from dts, step next if fail to protect no definition */
	resource = platform_get_resource(pdev, IORESOURCE_MEM, 2);
	ret = slimbus_asp_power_init(pdev, resource, pd);
	if (ret == -ENOMEM) {
		goto map_asp_err;
	} else if (ret == -EFAULT) {
		goto map_sctrl_err;
	}

	pdata = pd;
	pdata->dev = dev;
	ret = slimbus_pinctrl_init(pdev, pd);
	if (ret == get_err) {
		goto get_pinctrl_err;
	} else if (ret == ops_err) {
		goto ops_pinctrl_err;
	}

	pd->slimbus_dynamic_freq_enable = false;
	ret = of_property_read_string(pdev->dev.of_node, "slimbus_dynamic_freq", &property_value);
	if (!ret && (!strncmp(property_value, "true", 4))) {
		pd->slimbus_dynamic_freq_enable = true;
	}

	ret = of_property_read_string(pdev->dev.of_node, "platform-type", &platformtype);
	if (ret) {
		dev_err(dev, "of_property_read_string return error! ret:%d\n", ret);
		goto ops_pinctrl_err;
	}

	if (of_property_read_bool(dev->of_node, "switch_framer_disable"))
		pd->switch_framer_disable = true;

	if (of_property_read_bool(dev->of_node, "hi6402-soundtrigger-if1-used"))
		hi6402_soundtrigger_if1_used = true;

	pr_info("[%s:%d] platform type:%s device_type: %d, dynamic freq %d switch_framer_disable:%d!\n",
		__FUNCTION__, __LINE__, platformtype, pd->device_type, pd->slimbus_dynamic_freq_enable, pd->switch_framer_disable);

	ret = slimbus_init_platform_params(platformtype, pd->device_type, &(pd->platform_type));
	if (ret) {
		dev_err(dev, "slimbus platform param init fail\n");
		goto release_slimbusdev;
	}

	slimbus_module_enable(slimbus_devices[pd->device_type], true);

	ret = slimbus_drv_init(pd->platform_type, pd->base_addr, pd->asp_reg_base_addr, pd->irq);
	if (ret) {
		dev_err(dev, "slimbus drv init failed!\n");
		goto slimbus_err;
	}

	ret = slimbus_drv_bus_configure(&bus_config[SLIMBUS_BUS_CONFIG_NORMAL]);
	if (ret) {
		dev_err(dev, "slimbus bus configure failed!!\n");
		slimbus_drv_release(pd->irq);
		goto slimbus_err;
	}

	slimbus_dev = slimbus_devices[pd->device_type];

	pd->framerstate = SLIMBUS_FRAMER_SOC;
	pd->lastframer = SLIMBUS_FRAMER_SOC;
	platform_set_drvdata(pdev, pd);

	slimbus_pm_init(pdev, pd);

	return 0;

slimbus_err:
release_slimbusdev:
	if (pdata) {
		pdata->dev_ops->release_slimbus_device(slimbus_dev);
	}
ops_pinctrl_err:
	pinctrl_put(pd->pctrl);
get_pinctrl_err:
	regulator_disable(pd->regu_asp);
map_sctrl_err:
	iounmap(pd->sctrl_base_addr);
map_asp_err:
	iounmap(pd->asp_reg_base_addr);
	slimbus_utils_deinit();

	iounmap(pd->base_addr);

	clk_disable_unprepare(pd->asp_subsys_clk);

	clk_disable_unprepare(pd->pmu_audio_clk);

	pdata = NULL;

	return -EFAULT;
}

static int slimbus_remove(struct platform_device *pdev)
{
	int ret = 0;
	struct slimbus_private_data *pd = platform_get_drvdata(pdev);
	struct device	*dev = &pdev->dev;

	if (!pd) {
		pr_err("pd is null\n");
		return -EINVAL;
	}

	if (pd->pm_runtime_support) {
		pm_runtime_resume(dev);
		pm_runtime_disable(dev);
	}

	ret = slimbus_switch_framer(pd->device_type, SLIMBUS_FRAMER_SOC);
	if (ret) {
		dev_err(dev, "switch framer to SLIMBUS_DEVICE_SOC fail, ret:%d\n", ret);
	}

	slimbus_drv_release(pd->irq);

	pd->dev_ops->release_slimbus_device(slimbus_devices[pd->device_type]);

	pinctrl_put(pd->pctrl);
	ret = regulator_disable(pd->regu_asp);
	if (ret) {
		dev_err(dev, "regulator disable failed!, ret:%d\n", ret);
	}

	iounmap(pd->asp_reg_base_addr);
	slimbus_utils_deinit();
	iounmap(pd->base_addr);
	iounmap(pd->sctrl_base_addr);
	clk_disable_unprepare(pd->pmu_audio_clk);
	clk_disable_unprepare(pd->asp_subsys_clk);

	platform_set_drvdata(pdev, NULL);

	if (pd->pm_runtime_support) {
		pm_runtime_set_suspended(dev);
	}

	return 0;
}

static int slimbus_suspend(struct device *device)
{
	struct platform_device *pdev = to_platform_device(device);
	struct slimbus_private_data *pd = platform_get_drvdata(pdev);
	int ret = 0;
	int pm_ret = 0;
	uint32_t asppower = 0;
	uint32_t aspclk = 0;

	if (!pd) {
		pr_err("pd is null\n");
		return -EINVAL;
	}

	if (pd->pm_runtime_support) {
		pm_ret = pm_runtime_get_sync(device);
		if (pm_ret < 0) {
			pr_err("[%s:%d] pm resume error, pm_ret:%d\n", __FUNCTION__, __LINE__, pm_ret);
			BUG_ON(true);
			return pm_ret;
		}
	}

	pd->portstate = slimbus_port_state_get(pd->base_addr);
	pr_info("[%s:%d] portstate:0x%x usage_count:0x%x status:%x, pmuclk:%d aspclk:%d, --\n",__FUNCTION__, __LINE__,
		pd->portstate, atomic_read(&(device->power.usage_count)), device->power.runtime_status,
		clk_get_enable_count(pd->pmu_audio_clk), clk_get_enable_count(pd->asp_subsys_clk));

	if (!pd->portstate) {

		/* make sure last msg has been processed finished */
		mdelay(1);
		slimbus_int_need_clear_set(true);
		/*
		* while fm, hi64xx pll is in high freq, slimbus framer is in codec side
		* we need to switch to soc in this case, and switch to 64xx in resume
		*/

		if (pd->framerstate == SLIMBUS_FRAMER_CODEC) {
			pr_err("[%s:%d]switch framer to soc \n", __FUNCTION__, __LINE__);
			ret = slimbus_switch_framer(pd->device_type, SLIMBUS_FRAMER_SOC);
			if (ret) {
				pr_err("%s : slimbus switch framer failed!\n", __FUNCTION__);
				goto exit;
			}
			pd->lastframer = SLIMBUS_FRAMER_CODEC;
		} else {
			pd->lastframer = SLIMBUS_FRAMER_SOC;
		}

		ret = slimbus_pause_clock(pd->device_type, SLIMBUS_RT_UNSPECIFIED_DELAY);
		if (ret) {
			dev_err(device, "%s : slimbus pause clock failed, ret=%#x\n", __FUNCTION__, ret);
		}
		/* make sure para has updated */
		mdelay(1);

		ret = slimbus_drv_stop();
		if (ret) {
			pr_err("%s : slimbus stop failed!\n", __FUNCTION__);
		}

		/* set pin to  low power mode */
		ret = pinctrl_select_state(pd->pctrl, pd->pin_idle);
		if (ret) {
			dev_err(device, "%s : could not set pins to idle state\n", __FUNCTION__);
			goto exit;
		}

		clk_disable_unprepare(pd->pmu_audio_clk);

		ret = regulator_disable(pd->regu_asp);
		if (ret) {
			dev_err(device, "%s : regulator disable failed! \n", __FUNCTION__);
			goto exit;
		}

		clk_disable_unprepare(pd->asp_subsys_clk);
	}

exit:
	asppower = slimbus_asp_state_get(pd->sctrl_base_addr, pd->asp_power_state_offset);
	aspclk = slimbus_asp_state_get(pd->sctrl_base_addr, pd->asp_clk_state_offset);
	pr_info("[%s:%d] portstate:0x%x asppower:0x%x aspclkstate:0x%x usage_count:0x%x status:%x, pmuclk:%d aspclk:%d, --\n",__FUNCTION__, __LINE__,
		pd->portstate, asppower, aspclk, atomic_read(&(device->power.usage_count)), device->power.runtime_status,
		clk_get_enable_count(pd->pmu_audio_clk), clk_get_enable_count(pd->asp_subsys_clk));

	return ret;
}

static int slimbus_resume(struct device *device)
{
	struct platform_device *pdev = to_platform_device(device);
	struct slimbus_private_data *pd = platform_get_drvdata(pdev);
	int ret = 0;
	uint32_t asppower = 0;
	uint32_t aspclk = 0;

	if (!pd) {
		pr_err("pd is null\n");
		return -EINVAL;
	}
	asppower = slimbus_asp_state_get(pd->sctrl_base_addr, pd->asp_power_state_offset);
	aspclk = slimbus_asp_state_get(pd->sctrl_base_addr, pd->asp_clk_state_offset);

	pr_info("[%s:%d] portstate:0x%x asppower:0x%x aspclkstate:0x%x usage_count:0x%x status:%x, pmuclk:%d aspclk:%d, --\n",__FUNCTION__, __LINE__,
		pd->portstate, asppower, aspclk, atomic_read(&(device->power.usage_count)), device->power.runtime_status,
		clk_get_enable_count(pd->pmu_audio_clk), clk_get_enable_count(pd->asp_subsys_clk));

	if (!pd->portstate) {
		ret = clk_prepare_enable(pd->asp_subsys_clk);
		if (ret) {
			dev_err(device, "asp_subsys_clk :clk prepare enable failed !\n");
			goto exit;
		}

		ret = regulator_enable(pd->regu_asp);
		if (ret) {
			dev_err(device, "couldn't enable regulators %d\n", ret);
			goto exit;
		}

		ret = clk_prepare_enable(pd->pmu_audio_clk);
		if (ret) {
			dev_err(device, "pmu_audio_clk :clk prepare enable failed !\n");
			goto exit;
		}
		/* make sure pmu clk has stable */
		mdelay(1);

		ret = pinctrl_select_state(pd->pctrl, pd->pin_default);
		if (ret) {
			dev_err(device, "could not set pins to default state\n");
			goto exit;
		}

		slimbus_int_need_clear_set(false);

		slimbus_module_enable(slimbus_devices[pd->device_type], true);
		ret = slimbus_drv_resume_clock();
		if (ret) {
			dev_err(device, "slimbus resume clock failed, ret=%d\n", ret);
		}

		ret = slimbus_dev_init(pd->platform_type);
		if (ret) {
			dev_err(device, "slimbus drv init failed!\n");
			goto exit;
		}

		ret = slimbus_drv_bus_configure(&bus_config[SLIMBUS_BUS_CONFIG_NORMAL]);
		if (ret) {
			dev_err(device, "slimbus bus configure failed!!\n");
			goto exit;
		}

		if (pd->lastframer == SLIMBUS_FRAMER_CODEC) {
			ret = slimbus_switch_framer(pd->device_type, SLIMBUS_FRAMER_CODEC);
			pr_info("[%s:%d] switch_framer:%#x + \n", __FUNCTION__, __LINE__,  pdata->lastframer);
		}
	}

exit:
	if (pd->pm_runtime_support) {
		pm_runtime_mark_last_busy(device);
		pm_runtime_put_autosuspend(device);

		pm_runtime_disable(device);
		pm_runtime_set_active(device);
		pm_runtime_enable(device);
	}

	asppower = slimbus_asp_state_get(pd->sctrl_base_addr, pd->asp_power_state_offset);
	aspclk = slimbus_asp_state_get(pd->sctrl_base_addr, pd->asp_clk_state_offset);
	pr_info("[%s:%d] portstate:0x%x asppower:0x%x aspclkstate:0x%x usage_count:0x%x status:%x, pmuclk:%d aspclk:%d, --\n",__FUNCTION__, __LINE__,
		pd->portstate, asppower, aspclk, atomic_read(&(device->power.usage_count)), device->power.runtime_status,
		clk_get_enable_count(pd->pmu_audio_clk), clk_get_enable_count(pd->asp_subsys_clk));

	return ret;
}

void slimbus_pm_debug(void)
{
	uint32_t asppower = 0;
	uint32_t aspclk = 0;

	asppower = slimbus_asp_state_get(pdata->sctrl_base_addr, pdata->asp_power_state_offset);
	aspclk = slimbus_asp_state_get(pdata->sctrl_base_addr, pdata->asp_clk_state_offset);

	pr_info("[%s:%d] portstate:0x%x trackstate:0x%x asppower:0x%x aspclkstate:0x%x usage_count:0x%x status:%x clk:%d  ++\n",__FUNCTION__, __LINE__,
		pdata->portstate, slimbus_trackstate_get(), asppower, aspclk, atomic_read(&(pdata->dev->power.usage_count)), pdata->dev->power.runtime_status, clk_get_enable_count(pdata->pmu_audio_clk));
}

static int slimbus_runtime_suspend(struct device *device)
{
	struct platform_device *pdev = to_platform_device(device);
	struct slimbus_private_data *pd = platform_get_drvdata(pdev);
	int ret = 0;
	uint32_t asppower = 0;
	uint32_t aspclk = 0;

	if (!pd) {
		pr_err("pd is null\n");
		return -EINVAL;
	}

	pd->portstate = slimbus_port_state_get(pd->base_addr);

	pr_info("[%s:%d] portstate:0x%x trackstate:0x%x usage_count:0x%x status:%x pmuclk:%d aspclk:%d  ++\n",__FUNCTION__, __LINE__,
		pd->portstate, slimbus_trackstate_get(), atomic_read(&(device->power.usage_count)), device->power.runtime_status,
		clk_get_enable_count(pd->pmu_audio_clk), clk_get_enable_count(pd->asp_subsys_clk));

	if (pd->portstate != 0)
		pr_err("[%s:%d] portstate is nozero:0x%x\n", __FUNCTION__, __LINE__, pd->portstate);

	/* make sure last msg has been processed finished */
	mdelay(1);
	slimbus_int_need_clear_set(true);

	ret = slimbus_pause_clock(pd->device_type, SLIMBUS_RT_UNSPECIFIED_DELAY);
	if (ret) {
		pr_err("%s : slimbus pause clock failed, ret=%#x\n", __FUNCTION__, ret);
	}
	/* make sure para has updated */
	mdelay(1);

	clk_disable_unprepare(pd->pmu_audio_clk);

	clk_disable_unprepare(pd->asp_subsys_clk);

	asppower = slimbus_asp_state_get(pd->sctrl_base_addr, pd->asp_power_state_offset);
	aspclk = slimbus_asp_state_get(pd->sctrl_base_addr, pd->asp_clk_state_offset);
	pr_info("[%s:%d] portstate:0x%x asppower:0x%x aspclkstate:0x%x usage_count:0x%x status:%x, pmuclk:%d aspclk:%d, --\n",__FUNCTION__, __LINE__,
		pd->portstate, asppower, aspclk, atomic_read(&(device->power.usage_count)), device->power.runtime_status,
		clk_get_enable_count(pd->pmu_audio_clk), clk_get_enable_count(pd->asp_subsys_clk));

	return ret;
}

static int slimbus_runtime_resume(struct device *device)
{
	struct platform_device *pdev = to_platform_device(device);
	struct slimbus_private_data *pd = platform_get_drvdata(pdev);
	int ret = 0;
	uint32_t asppower = 0;
	uint32_t aspclk = 0;

	if (!pd) {
		pr_err("pd is null\n");
		return -EINVAL;
	}

	asppower = slimbus_asp_state_get(pd->sctrl_base_addr, pd->asp_power_state_offset);
	aspclk = slimbus_asp_state_get(pd->sctrl_base_addr, pd->asp_clk_state_offset);

	pr_info("[%s:%d] portstate:0x%x asppower:0x%x aspclkstate:0x%x usage_count:0x%x status:%x pmuclk:%d aspclk:%d ++\n",__FUNCTION__, __LINE__,
		pd->portstate, asppower, aspclk, atomic_read(&(device->power.usage_count)), device->power.runtime_status,
		clk_get_enable_count(pd->pmu_audio_clk), clk_get_enable_count(pd->asp_subsys_clk));

	if (pd->portstate != 0)
		pr_err("[%s:%d] portstate is nozero:0x%x\n", __FUNCTION__, __LINE__, pd->portstate);

	ret = clk_prepare_enable(pd->asp_subsys_clk);
	if (ret) {
		pr_err("asp_subsys_clk :clk prepare enable failed !\n");
		goto exit;
	}

	ret = clk_prepare_enable(pd->pmu_audio_clk);
	if (ret) {
		pr_err("pmu_audio_clk :clk prepare enable failed !\n");
		goto exit;
	}
	/* make sure pmu clk has stable */
	mdelay(1);

	slimbus_int_need_clear_set(false);

	ret = slimbus_drv_resume_clock();
	if (ret) {
		pr_err("slimbus resume clock failed, ret=%d\n", ret);
	}

exit:
	asppower = slimbus_asp_state_get(pd->sctrl_base_addr, pd->asp_power_state_offset);
	aspclk = slimbus_asp_state_get(pd->sctrl_base_addr, pd->asp_clk_state_offset);
	pr_info("[%s:%d] portstate:0x%x asppower:0x%x aspclkstate:0x%x usage_count:0x%x status:%x, pmuclk:%d aspclk:%d, --\n",__FUNCTION__, __LINE__,
		pd->portstate, asppower, aspclk, atomic_read(&(device->power.usage_count)), device->power.runtime_status,
		clk_get_enable_count(pd->pmu_audio_clk), clk_get_enable_count(pd->asp_subsys_clk));

	return ret;
}

static const struct dev_pm_ops slimbus_pm_ops = {
	SET_SYSTEM_SLEEP_PM_OPS(slimbus_suspend, slimbus_resume)
	SET_RUNTIME_PM_OPS(slimbus_runtime_suspend, slimbus_runtime_resume, NULL)
};

static const struct of_device_id slimbus_match[] = {
	{
		.compatible = "candance,slimbus",
	},
	{},
};
MODULE_DEVICE_TABLE(of, slimbus_match);

static struct platform_driver slimbus_driver = {
	.probe = slimbus_probe,
	.remove = slimbus_remove,
	.driver = {
		.name = "hisilicon,slimbus",
		.owner = THIS_MODULE,
		.pm = &slimbus_pm_ops,
		.of_match_table = slimbus_match,
	},
};

static int __init slimbus_init(void)
{
	int ret;

	ret = platform_driver_register(&slimbus_driver);
	if (ret) {
		pr_err("driver register failed\n");
	}

	return ret;
}

static void __exit slimbus_exit(void)
{
	platform_driver_unregister(&slimbus_driver);
}
fs_initcall(slimbus_init);
module_exit(slimbus_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Hisilicon");

