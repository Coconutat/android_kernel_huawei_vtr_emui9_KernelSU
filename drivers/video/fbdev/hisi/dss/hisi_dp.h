/* Copyright (c) 2013-2014, Hisilicon Tech. Co., Ltd. All rights reserved.
*
* This program is free software; you can redistribute it and/or modify
* it under the terms of the GNU General Public License version 2 and
* only version 2 as published by the Free Software Foundation.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.	See the
* GNU General Public License for more details.
*
*/
#ifndef HISI_DP_H
#define HISI_DP_H

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/interrupt.h>
#include <linux/pci.h>
#include <linux/switch.h>

#include "dp/drm_dp_helper.h"
#include "dp/drm_dp_helper_additions.h"
#include "dp/reg.h"
#include "dp/dbg.h"
#include <huawei_platform/dp_source/dp_dsm.h>
#include <huawei_platform/dp_source/dp_factory.h>
#include <huawei_platform/dp_source/dp_debug.h>

#define CONFIG_DP_HDCP_ENABLE
//#define CONFIG_DP_GENERATOR_REF
#define CONFIG_DP_EDID_DEBUG
#define CONFIG_DP_SETTING_COMBOPHY 1
#define DP_TIME_INFO_SIZE		(24)
#define DP_MONTIOR_NAME_SIZE (11)

#define DP_PLUG_TYPE_NORMAL 0
#define DP_PLUG_TYPE_FLIPPED 1

#define DPTX_RECEIVER_CAP_SIZE	(0x100)
#define DPTX_SDP_NUM		(0x10)
#define DPTX_SDP_LEN	(0x9)
#define DPTX_SDP_SIZE	(9 * 4)
#define DPTX_DEFAULT_EDID_BUFLEN	(128UL)

/* The max rate and lanes supported by the core */
#define DPTX_MAX_LINK_RATE	DPTX_PHYIF_CTRL_RATE_HBR2
#define DPTX_MAX_LINK_LANES	(4)

/* The default rate and lanes to use for link training */
#define DPTX_DEFAULT_LINK_RATE DPTX_MAX_LINK_RATE
#define DPTX_DEFAULT_LINK_LANES DPTX_MAX_LINK_LANES

#define DP_SYSFS_ATTRS_NUM	(10)

#define DPTX_HDCP_REG_DPK_CRC_ORIG	0x331c1169
#define DPTX_HDCP_MAX_AUTH_RETRY	10
#define DPTX_HDCP_MAX_REPEATER_AUTH_RETRY 5
#define DPTX_COMBOPHY_PARAM_NUM		21


#define DPTX_AUX_TIMEOUT	(2000)

#define DEFAULT_AUXCLK_DPCTRL_RATE	16000000UL
#define DEFAULT_ACLK_DPCTRL_RATE_ES	288000000UL
#ifdef CONFIG_HISI_FB_V501
#define DEFAULT_ACLK_DPCTRL_RATE 207500000UL
#define DEFAULT_MIDIA_PPLL7_CLOCK_FREQ 1290000000UL
#define KIRIN_VCO_MIN_FREQ_OUPUT         800000 /*dssv501: 800 * 1000*/
#define KIRIN_SYS_FREQ   38400 /*dssv501: 38.4 * 1000 */
#define MIDIA_PPLL7_CTRL0	0x530
#define MIDIA_PPLL7_CTRL1	0x534
#define MIDIA_PERI_CTRL4	0x350
#define TX_VBOOST_ADDR		0x21
#define PERI_VOLTAGE_065V_CLK 300000000UL
#define PERI_VOLTAGE_070V_CLK 415000000UL
#define PERI_VOLTAGE_080V_CLK 594000000UL
#define PPLL7_DIV_VOLTAGE_065V 5
#define PPLL7_DIV_VOLTAGE_070V 4
#define PPLL7_DIV_VOLTAGE_080V 3
#else
#define DEFAULT_ACLK_DPCTRL_RATE	208000000UL
#define DEFAULT_MIDIA_PPLL7_CLOCK_FREQ	1782000000UL
#define KIRIN_VCO_MIN_FREQ_OUPUT         1000000 /*Boston: 1000 * 1000*/
#define KIRIN_SYS_FREQ   19200 /*Boston: 19.2f * 1000 */
#define MIDIA_PPLL7_CTRL0	0x50c
#define MIDIA_PPLL7_CTRL1	0x510
#define MIDIA_PERI_CTRL4	0x350
#define TX_VBOOST_ADDR		0xf
#define PERI_VOLTAGE_065V_CLK 255000000UL
#define PERI_VOLTAGE_070V_CLK 415000000UL
#define PERI_VOLTAGE_080V_CLK 594000000UL
#define PPLL7_DIV_VOLTAGE_065V 7
#define PPLL7_DIV_VOLTAGE_070V 5
#define PPLL7_DIV_VOLTAGE_080V 3
#endif

#define DEFAULT_MIDIA_PPLL7_CLOCK_FREQ_SAVEMODE	223000000UL

#define MAX_DIFF_SOURCE_X_SIZE	1920



#define MIDIA_PPLL7_FREQ_DEVIDER_MASK	GENMASK(25, 2)
#define MIDIA_PPLL7_FRAC_MODE_MASK	GENMASK(25, 0)
#define PMCTRL_PERI_CTRL4_TEMPERATURE_MASK		GENMASK(27, 26)
#define PMCTRL_PERI_CTRL4_TEMPERATURE_SHIFT		26
#define NORMAL_TEMPRATURE 0

#define ACCESS_REGISTER_FN_MAIN_ID_HDCP           0xc500aa01
#define ACCESS_REGISTER_FN_SUB_ID_HDCP_CTRL   (0x55bbccf1)
#define ACCESS_REGISTER_FN_SUB_ID_HDCP_INT   (0x55bbccf2)

/* #define DPTX_DEVICE_INFO(pdev) platform_get_drvdata(pdev)->panel_info->dp */

#define DPTX_CHANNEL_NUM_MAX 8
#define DPTX_DATA_WIDTH_MAX 24

enum dp_tx_hpd_states {
	HPD_OFF,
	HPD_ON,
};

enum pixel_enc_type {
	RGB = 0,
	YCBCR420 = 1,
	YCBCR422 = 2,
	YCBCR444 = 3,
	YONLY = 4,
	RAW = 5

};

enum color_depth {
	COLOR_DEPTH_INVALID = 0,
	COLOR_DEPTH_6 = 6,
	COLOR_DEPTH_8 = 8,
	COLOR_DEPTH_10 = 10,
	COLOR_DEPTH_12 = 12,
	COLOR_DEPTH_16 = 16
};

enum pattern_mode {
	TILE = 0,
	RAMP = 1,
	CHESS = 2,
	COLRAMP = 3
};

enum dynamic_range_type {
	CEA = 1,
	VESA = 2
};

enum colorimetry_type {
	ITU601 = 1,
	ITU709 = 2
};

enum video_format_type {
	VCEA = 0,
	CVT = 1,
	DMT = 2
};

enum iec_samp_freq_value {
	IEC_SAMP_FREQ_44K = 0,
	IEC_SAMP_FREQ_48K = 2,
	IEC_SAMP_FREQ_32K = 3,
	IEC_SAMP_FREQ_88K = 8,
	IEC_SAMP_FREQ_96K = 10,
	IEC_SAMP_FREQ_176K = 12,
	IEC_SAMP_FREQ_192K = 14,
};

enum iec_orig_samp_freq_value {
	IEC_ORIG_SAMP_FREQ_192K = 1,
	IEC_ORIG_SAMP_FREQ_176K = 3,
	IEC_ORIG_SAMP_FREQ_96K = 5,
	IEC_ORIG_SAMP_FREQ_88K = 7,
	IEC_ORIG_SAMP_FREQ_32K = 12,
	IEC_ORIG_SAMP_FREQ_48K = 13,
	IEC_ORIG_SAMP_FREQ_44K = 15,
};

/**
 * struct dptx_link - The link state.
 * @status: Holds the sink status register values.
 * @trained: True if the link is successfully trained.
 * @rate: The current rate that the link is trained at.
 * @lanes: The current number of lanes that the link is trained at.
 * @preemp_level: The pre-emphasis level used for each lane.
 * @vswing_level: The vswing level used for each lane.
 */
struct dptx_link {
	uint8_t status[DP_LINK_STATUS_SIZE];
	bool trained;
	uint8_t rate;
	uint8_t lanes;
	uint8_t preemp_level[4];
	uint8_t vswing_level[4];
};

/**
 * struct dptx_aux - The aux state
 * @sts: The AUXSTS register contents.
 * @data: The AUX data register contents.
 * @event: Indicates an AUX event ocurred.
 * @abort: Indicates that the AUX transfer should be aborted.
 */
struct dptx_aux {
	uint32_t sts;
	uint32_t data[4];
	atomic_t event;
	atomic_t abort;
};

struct sdp_header {
	uint8_t HB0;
	uint8_t HB1;
	uint8_t HB2;
	uint8_t HB3;
} __packed;

struct sdp_full_data {
	uint8_t en;
	uint32_t payload[9];
	uint8_t blanking;
	uint8_t cont;
} __packed;

struct hdcp_aksv {
	uint32_t lsb;
	uint32_t msb;
};

struct hdcp_dpk {
	uint32_t lsb;
	uint32_t msb;
};

struct hdcp_params {
	struct hdcp_aksv aksv;
	struct hdcp_dpk dpk[40];
	uint32_t enc_key;
	uint32_t crc32;
	uint32_t auth_fail_count;
	uint32_t hdcp13_is_en;
};

typedef struct hdcp13_int_params {
	u8 auth_fail_count;
	int hdcp13_is_en;
}hdcp13_int_params_t;

enum _master_hdcp_op_type_ {
	DSS_HDCP13_ENABLE = 1,
	DSS_HDCP22_ENABLE,
	DSS_HDCP13_ENCRYPT_ENABLE,
	DSS_HDCP_OBS_SET,
	DSS_HDCP_INT_CLR,
	DSS_HDCP_INT_MASK,
	DSS_HDCP_CP_IRQ,
	DSS_HDCP_DPC_SEC_EN,
	DSS_HDCP_ENC_MODE_EN,
	HDCP_OP_SECURITY_MAX,
};

struct audio_params {
	uint8_t iec_channel_numcl0;
	uint8_t iec_channel_numcr0;
	uint8_t use_lut;
	uint8_t iec_samp_freq;
	uint8_t iec_word_length;
	uint8_t iec_orig_samp_freq;
	uint8_t data_width;
	uint8_t num_channels;
	uint8_t inf_type;
	uint8_t mute;
	uint8_t ats_ver;
};

struct dtd {
	uint64_t pixel_clock;
	uint16_t pixel_repetition_input;
	uint16_t h_active;
	uint16_t h_blanking;
	uint16_t h_image_size;
	uint16_t h_sync_offset;
	uint16_t h_sync_pulse_width;
	uint8_t h_sync_polarity;
	uint16_t v_active;
	uint16_t v_blanking;
	uint16_t v_image_size;
	uint16_t v_sync_offset;
	uint16_t v_sync_pulse_width;
	uint8_t v_sync_polarity;
	uint8_t interlaced; /* 1 for interlaced, 0 progressive */
};

struct video_params {
	enum pixel_enc_type pix_enc;
	enum pattern_mode pattern_mode;
	struct dtd mdtd;
	uint8_t mode;
	enum color_depth bpc;
	enum colorimetry_type colorimetry;
	enum dynamic_range_type dynamic_range;
	uint8_t aver_bytes_per_tu;
	uint8_t aver_bytes_per_tu_frac;
	uint8_t init_threshold;
	uint32_t refresh_rate;
	uint8_t video_format;
	uint8_t m_fps;
};

/*edid info*/
struct timing_info
{
	struct list_head list_node;

	uint8_t vSyncPolarity;
	uint8_t hSyncPolarity;

	uint64_t pixelClock;
	uint16_t hActivePixels;
	uint16_t hBlanking;
	uint16_t hSyncOffset;
	uint16_t hSyncPulseWidth;
	uint16_t hBorder;
	uint16_t hSize;
	uint16_t vActivePixels;
	uint16_t vBlanking;
	uint16_t vSyncOffset;
	uint16_t vSyncPulseWidth;
	uint16_t vBorder;
	uint16_t vSize;
	uint16_t inputType;
	uint16_t interlaced;
	uint16_t syncScheme;
	uint16_t schemeDetail;
};

struct ext_timing
{
	uint16_t extFormatCode;
	uint16_t extHPixels;
	uint16_t extVPixels;
	uint16_t extVFreq;
};

struct edid_video
{
	uint8_t mainVCount;
	uint8_t extVCount;

	uint16_t maxHPixels;
	uint16_t maxVPixels;
	uint16_t maxHImageSize;
	uint16_t maxVImageSize;
	uint16_t maxVFreq;
	uint16_t minVFreq;
	uint16_t maxHFreq;
	uint16_t minHFreq;
	uint16_t maxPixelClock;

	struct list_head *dptx_timing_list;
	struct ext_timing *extTiming;
	char *dp_monitor_descriptor;
};

struct edid_audio_info
{
	uint16_t format;
	uint16_t channels;
	uint16_t sampling;
	uint16_t bitrate;
};

struct edid_audio
{
	struct edid_audio_info *spec;
	uint8_t basicAudio;
	uint8_t extSpeaker;
	uint8_t extACount;
};

struct edid_information
{
	struct edid_video Video;
	struct edid_audio Audio;
};

/**
 * struct dp_ctrl - The representation of the DP TX core
 * @mutex:
 * @base: Base address of the registers
 * @irq: IRQ number
 * @version: Contents of the IP_VERSION register
 * @max_rate: The maximum rate that the controller supports
 * @max_lanes: The maximum lane count that the controller supports
 * @dev: The struct device
 * @root: The debugfs root
 * @regset: The debugfs regset
 * @vparams: The video params to use
 * @aparams: The audio params to use
 * @waitq: The waitq
 * @shutdown: Signals that the driver is shutting down and that all
 *            operations should be aborted.
 * @c_connect: Signals that a HOT_PLUG or HOT_UNPLUG has occurred.
 * @sink_request: Signals the a HPD_IRQ has occurred.
 * @rx_caps: The sink's receiver capabilities.
 * @edid: The sink's EDID.
 * @aux: AUX channel state for performing an AUX transfer.
 * @link: The current link state.
 */
struct dp_ctrl {
	struct mutex dptx_mutex; /* generic mutex for dptx */

	void __iomem *base;
	uint32_t irq;

	uint32_t version;
	uint8_t max_rate;
	uint8_t max_lanes;

	struct device *dev;
	struct switch_dev sdev;
	struct switch_dev dp_switch;
	struct hisi_fb_data_type *hisifd;

	struct video_params vparams;
	struct audio_params aparams;
	struct hdcp_params hparams;

	struct edid_information edid_info;

	wait_queue_head_t dptxq;
	wait_queue_head_t waitq;

	atomic_t shutdown;
	atomic_t c_connect;
	atomic_t sink_request;

	uint8_t rx_caps[DPTX_RECEIVER_CAP_SIZE];

	uint8_t *edid;
	uint32_t edid_try_count;
	uint32_t edid_try_delay; // unit: ms

	struct sdp_full_data sdp_list[DPTX_SDP_NUM];
	struct dptx_aux aux;
	struct dptx_link link;

	bool dptx_vr;
	bool dptx_gate;
	bool dptx_enable;
	bool dptx_plug_type;
	bool dptx_detect_inited;
	bool same_source;
	bool video_transfer_enable;

	uint8_t detect_times;
	uint8_t current_link_rate;
	uint8_t current_link_lanes;
	uint32_t hpd_state;
	uint32_t user_mode;
	uint32_t combophy_pree_swing[DPTX_COMBOPHY_PARAM_NUM];
	uint32_t max_edid_timing_hactive;
	enum video_format_type user_mode_format;
	int sysfs_index;
	struct attribute *sysfs_attrs[DP_SYSFS_ATTRS_NUM];
	struct attribute_group sysfs_attr_group;

	struct hrtimer dptx_hrtimer;
	struct workqueue_struct *dptx_check_wq;
	struct work_struct dptx_check_work;
};

static inline uint32_t dptx_readl(struct dp_ctrl *dp, uint32_t offset)
{
	uint32_t data = readl(dp->base + offset);
	return data;
}

static inline void dptx_writel(struct dp_ctrl *dp, uint32_t offset, uint32_t data)
{
	writel(data, dp->base + offset);
}

/*
 * Wait functions
 */
#define dptx_wait(_dptx, _cond, _timeout)				\
	({								\
		int __retval;						\
		__retval = wait_event_interruptible_timeout(		\
			_dptx->waitq,					\
			((_cond) || (atomic_read(&_dptx->shutdown))),	\
			msecs_to_jiffies(_timeout));			\
		if (atomic_read(&_dptx->shutdown)) {			\
			__retval = -ESHUTDOWN;				\
		}							\
		else if (!__retval) {					\
			__retval = -ETIMEDOUT;				\
		}							\
		__retval;						\
	})


void dptx_notify_shutdown(struct dp_ctrl *dptx);
void dp_send_cable_notification(struct dp_ctrl *dptx, int val);

struct hisi_fb_data_type;
int hisi_dp_hpd_register(struct hisi_fb_data_type *hisifd);
void hisi_dp_hpd_unregister(struct hisi_fb_data_type *hisifd);

#endif  /* HISI_DP_H */
