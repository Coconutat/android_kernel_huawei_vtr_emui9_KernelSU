/* Copyright (c) 2008-2019, Huawei Tech. Co., Ltd. All rights reserved.
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
#include <linux/module.h>
#include <linux/init.h>
#include <linux/regmap.h>
#include <linux/delay.h>
#include <linux/device.h>
#include <linux/kernel.h>
#include <linux/of.h>
#include <linux/kobject.h>
#include <linux/slab.h>
#include <linux/string.h>
#include <linux/timer.h>
#include <linux/timex.h>
#include <linux/rtc.h>
#include <linux/types.h>
#include <linux/list.h>
#include <dsm/dsm_pub.h>
#include <huawei_platform/log/hw_log.h>
#include <huawei_platform/log/imonitor.h>
#include <huawei_platform/dp_source/dp_dsm.h>
#include <huawei_platform/dp_source/dp_debug.h>
#include <huawei_platform/dp_source/dp_factory.h>

#ifdef DP_DSM_ENABLE
#undef HWLOG_TAG
#define HWLOG_TAG dp_dsm
HWLOG_REGIST();

#define DP_DSM_NAME     "dsm_dp"
#define DP_DSM_BUF_SIZE (1024)

// for print debug log
//#define DP_DSM_DEBUG

#ifndef UNUSED
#define UNUSED(x) ((void)(x))
#endif

#ifndef MIN
#define MIN(x,y)  ((x) < (y) ? (x) : (y))
#endif

#ifndef MAX
#define MAX(x,y)  ((x) > (y) ? (x) : (y))
#endif

#ifndef ARRAY_SIZE
#define ARRAY_SIZE(a) (sizeof(a)/sizeof(a[0]))
#endif

#define DP_DSM_DATETIME_SIZE         (20)
#define DP_DSM_TIME_YEAR_OFFSET      (1900)
#define DP_DSM_TIME_MONTH_OFFSET     (1)
#define DP_DSM_TIME_US_OF_MS         (1000) // 1ms = 1000us
#define DP_DSM_TIME_MS_OF_SEC        (1000) // 1s = 1000ms
#define DP_DSM_TIME_SECOND_OF_MINUTE (60)
#define DP_DSM_TIME_SECOND_OF_HOUR   (60 * DP_DSM_TIME_SECOND_OF_MINUTE)
#define DP_DSM_TIME_SECOND_OF_DAY    (24 * DP_DSM_TIME_SECOND_OF_HOUR)

// for datetime type
#define DP_DSM_TIMEVAL_WITH_TZ(t)    t.tv_sec += (sys_tz.tz_minuteswest * DP_DSM_TIME_SECOND_OF_MINUTE)
#define DP_DSM_TIME_MS_PERCENT(t)    ((t) / 100) // unit: ms -> 100ms

// EDID 1.3 data format
// 18 EDID version, usually 1 (for 1.3)
// 19 EDID revision, usually 3 (for 1.3)
#define DP_DSM_EDID_VERSION_OFFSET     (18)
#define DP_DSM_EDID_REVISION_OFFSET    (19)
#define DP_DSM_EDID_VERSION_MAKE(a, b) (((a) << 8) | (b))

// DPCD data structure revision number.
// DPCD_REV: 00000h
// 10h = DPCD Rev. 1.0.
// 11h = DPCD Rev. 1.1.
// 12h = DPCD Rev. 1.2.
// 13h = DPCD Rev. 1.3 (for eDP v1.4 DPRX only).
// 14h = DPCD Rev. 1.4.
// 3:0 Minor Revision Number
// 7:4 Major Revision Number
#define DP_DSM_DPCD_REVISION_OFFSET (4)
#define DP_DSM_DPCD_REVISION_MASK   (0x0F)

// Manufacturer ID.
// These IDs are assigned by Microsoft, they are PNP IDs "00001=A§; ※00010=B§; ... ※11010=Z§.
// Bit 7 (at address 08h) is 0
// the first character (letter) is located at bits 6 -> 2 (at address 08h),
// the second character (letter) is located at bits 1 & 0 (at address 08h) and bits 7 -> 5 (at address 09h),
// and the third character (letter) is located at bits 4 -> 0 (at address 09h).
#define DP_DSM_MID_LETTER_NUM    (3)
#define DP_DSM_MID_LETTER_OFFSET (5)
#define DP_DSM_MID_LETTER_MASK   (0x1F)

#define DP_DSM_MID_INFO_SIZE     (DP_DSM_MID_LETTER_NUM + 1)
#define DP_DSM_MID_OFFSET        (8) // offset in edid block

#define DP_DSM_MID_LETTER(a, b)  ((a << 8) | b)

// Descriptor blocks.
// Detailed timing descriptors, in decreasing preference order.
// After all detailed timing descriptors, additional descriptors are permitted:
// * Monitor range limits (required)
// * ASCII text (monitor name (required), monitor serial number or unstructured text)
// * 6 Additional standard timing information blocks
// * Colour point data
// 54每71	Descriptor 1
// 72每89	Descriptor 2
// 90每107	Descriptor 3
// 108每125	Descriptor 4
#define DP_DSM_EDID_DESC_OFFSET         (0x36) // from 54 bytes
#define DP_DSM_EDID_DESC_NUM            (4)
#define DP_DSM_EDID_DESC_SIZE           (18)

#define DP_DSM_EDID_DESC_MASK_OFFSET    (3)
#define DP_DSM_EDID_DESC_MONITOR_MASK   (0xfc)
#define DP_DSM_EDID_DESC_MONITOR_OFFSET (5)
#define DP_DSM_EDID_DESC_MONITOR_TAIL   (0x0a)

#define DP_DSM_MONTIOR_INFO_SIZE        (16)
#define DP_DSM_TU_FAIL                  (65)

#define DP_DSM_TYPEC_IN_OUT_NUM          (8)
#define DP_DSM_TCA_DEV_TYPE_NUM          (32)
#define DP_DSM_IRQ_VECTOR_NUM            (32)
#define DP_DSM_SOURCE_SWITCH_NUM         (8)
#define DP_DSM_EARLIEST_TIME_POINT(a, b) (((a) >= (b)) ? ((a) % (b)) : 0)

// error number:
// 1. hotplug retval
#define DP_DSM_ERRNO_DSS_PWRON_FAILED    (0xFFFF)
#define DP_DSM_ERRNO_HPD_NOT_EXISTED     (0xFFFF + 1)
// 2. link training retval
#define DP_DSM_ERRNO_DEVICE_SRS_FAILED   (0xFFFF)

// from tca.h (vendor\hisi\ap\kernel\include\linux\hisi\contexthub)
typedef enum _tca_irq_type_e {
	TCA_IRQ_HPD_OUT = 0,
	TCA_IRQ_HPD_IN = 1,
	TCA_IRQ_SHORT = 2,
	TCA_IRQ_MAX_NUM
} TCA_IRQ_TYPE_E;

typedef enum {
	TCPC_NC = 0,
	TCPC_USB31_CONNECTED = 1,
	TCPC_DP = 2,
	TCPC_USB31_AND_DP_2LINE = 3,
	TCPC_MUX_MODE_MAX
} TCPC_MUX_CTRL_TYPE;

typedef enum _tca_device_type_e {
	TCA_CHARGER_CONNECT_EVENT = 0, /*usb device in*/
	TCA_CHARGER_DISCONNECT_EVENT,  /*usb device out*/
	TCA_ID_FALL_EVENT,             /*usb host in*/
	TCA_ID_RISE_EVENT,             /*usb host out*/
	TCA_DP_OUT,
	TCA_DP_IN,
	TCA_DEV_MAX
} TCA_DEV_TYPE_E;

typedef enum _typec_plug_orien_e {
	TYPEC_ORIEN_POSITIVE = 0,
	TYPEC_ORIEN_NEGATIVE = 1,
	TYPEC_ORIEN_MAX
} TYPEC_PLUG_ORIEN_E;

// hpd or short irq
typedef struct {
	struct list_head list;
	struct timeval time;

	TCA_IRQ_TYPE_E     irq_type;
	TCPC_MUX_CTRL_TYPE cur_mode;
	TCPC_MUX_CTRL_TYPE mode_type;
	TCA_DEV_TYPE_E     dev_type;
	TYPEC_PLUG_ORIEN_E typec_orien;
} dp_connected_event_t;

typedef enum {
	HDCP_KEY_INVALID,
	HDCP_KEY_SUCCESS,
	HDCP_KEY_FAILED,
} dp_hdcp_key_t;

typedef union {
	uint32_t vswing_preemp;
	struct {
		uint8_t preemp:4; // low 4 bits
		uint8_t vswing:4; // high 4 bits
	} vs_pe[DP_DSM_VS_PE_NUM];
} dp_vs_pe_t;

typedef enum {
	EDID_CHECK_SUCCESS,
	EDID_CHECK_FAILED,
} dp_edid_check_result_t;

typedef union {
	struct {
		uint32_t header_num;
		uint32_t checksum_num;
		uint32_t header_flag;
		uint32_t checksum_flag;
		uint8_t  ext_blocks_num;
		uint8_t  ext_blocks;
	} u32;

	struct {
		uint8_t header_num[DP_PARAM_EDID_NUM];
		uint8_t checksum_num[DP_PARAM_EDID_NUM];
		uint8_t header_flag[DP_PARAM_EDID_NUM];
		uint8_t checksum_flag[DP_PARAM_EDID_NUM];
		uint8_t ext_blocks_num;
		uint8_t ext_blocks;
	} u8;
} dp_edid_check_t;

// dp hotplug info
typedef struct {
	struct list_head list;
	struct timeval time[DP_PARAM_TIME_NUM];

	// dp info
	char mid[DP_DSM_MID_INFO_SIZE]; // manufacturer id
	char monitor[DP_DSM_MONTIOR_INFO_SIZE];

	uint16_t width;
	uint16_t high;
	uint16_t max_width;
	uint16_t max_high;
	int pixel_clock;
	uint8_t  fps;
	uint8_t  max_rate;  // max_lanes_rate
	uint8_t  max_lanes;
	uint8_t  rate;      // link_lanes_rate
	uint8_t  lanes;
	int tu;
	int tu_fail;
	dp_vs_pe_t vp; // vs_pe

	bool source_mode;    // same_source or diff_source
	uint8_t user_mode;   // vesa_id
	uint8_t user_format; // vcea, cvt, dmt
	uint8_t basic_audio;

	uint16_t edid_version;
	uint8_t  dpcd_revision;

	bool safe_mode;
	bool no_hotunplug;
	int read_edid_retval;
	int link_training_retval;
	int hotplug_retval;

	// extend info
	int current_edid_num;
	dp_edid_check_t edid_check;
	uint8_t edid[DP_PARAM_EDID_NUM][DP_DSM_EDID_BLOCK_SIZE];

	// for debug
	uint8_t vs_force;
	uint8_t pe_force;
	dp_vs_pe_t vp_force;
} dp_hotplug_info_t;

typedef struct {
	struct list_head list;

	uint16_t h_active;
	uint16_t v_active;
	uint32_t pixel_clock;
	uint8_t  vesa_id;
} dp_timing_info_t;

typedef struct {
	struct timeval time;

	bool rw;
	bool i2c;
	uint32_t addr;
	uint32_t len;
	int retval;
} dp_aux_rw_t;

typedef struct {
	int in_out_num;
	struct timeval in_time[DP_DSM_TYPEC_IN_OUT_NUM];
	struct timeval out_time[DP_DSM_TYPEC_IN_OUT_NUM];
} dp_typec_in_out_t;

typedef struct {
	struct list_head list;
	dp_imonitor_type_t type;

	// typec cable connected
	struct timeval tpyec_in_time;
	struct timeval tpyec_out_time;
	struct timeval link_min_time;
	struct timeval link_max_time;
	struct timeval same_mode_time;
	struct timeval diff_mode_time;
	int event_num;
	TCPC_MUX_CTRL_TYPE tpyec_cable_type;
	struct list_head event_list_head;

	// dp in or out
	int irq_hpd_num;
	int irq_short_num;
	int dp_in_num;
	int dp_out_num;
	uint8_t tca_dev_type[DP_DSM_TCA_DEV_TYPE_NUM];

	// from handle_sink_request()
	int link_retraining_num;

	// dp hotplug
	bool need_add_hotplug_to_list;
	dp_hotplug_info_t *cur_hotplug_info;
	struct list_head hotplug_list_head;
	struct list_head timing_list_head;

	bool is_hotplug_running;
	uint32_t hotplug_state;
	uint32_t last_hotplug_state;
	uint16_t hotplug_width;
	uint16_t hotplug_high;

	bool cur_source_mode;
	bool dss_pwron_failed;
	int link_training_retval;
	int link_retraining_retval;
	int hotplug_retval;
	dp_aux_rw_t aux_rw;

	int irq_vector_num;
	uint8_t irq_vector[DP_DSM_IRQ_VECTOR_NUM];
	int source_switch_num;
	uint8_t source_switch[DP_DSM_SOURCE_SWITCH_NUM];

	// hdcp
	struct timeval hdcp_time;
	uint8_t hdcp_version; // hdcp1.3 or hdcp2.2
	uint8_t hdcp_key;

	// for report control flag
	bool rcf_basic_info_of_same_mode;
	bool rcf_basic_info_of_diff_mode;
	bool rcf_extend_info;
	bool rcf_link_training_info;
	bool rcf_link_retraining_info;
	bool rcf_hotplug_info;
	bool rcf_hdcp_info;

	// for repeated event recognition
	int hpd_repeated_num;

	// err_count: from dpcd regs in sink devices
	uint16_t lane0_err;
	uint16_t lane1_err;
	uint16_t lane2_err;
	uint16_t lane3_err;

#ifdef DP_DEBUG_ENABLE
	bool need_lanes_force;
	bool need_rate_force;
	bool need_resolution_force;
	uint8_t lanes_force;
	uint8_t rate_force;
	uint8_t user_mode;
	uint8_t user_format;
#endif
} dp_dsm_priv_t;

#ifndef DP_DSM_DEBUG
#ifndef DP_DEBUG_ENABLE
#define DP_DSM_REPORT_NUM_IN_ONE_DAY (5)   // for user version
#else
#define DP_DSM_REPORT_NUM_IN_ONE_DAY (1000) // for eng version
#endif // DP_DEBUG_ENABLE
#define DP_DSM_REPORT_TIME_INTERVAL  (DP_DSM_TIME_SECOND_OF_DAY * DP_DSM_TIME_MS_OF_SEC)
#else
#define DP_DSM_REPORT_NUM_IN_ONE_DAY (5)
#define DP_DSM_REPORT_TIME_INTERVAL  (300 * 1000) // 5 minutes
#endif // DP_DSM_DEBUG
#define DP_DSM_IMONITOR_DELAYED_TIME (0)
#define DP_DSM_IMONITOR_PREPARE_PRIV ((NULL == data) ? g_dp_dsm_priv : data)

#define DP_DSM_HPD_DELAYED_TIME  (3 * 1000)  // 3s
#define DP_DSM_HPD_TIME_INTERVAL (10 * 1000) // 10s

#define DP_DSM_GET_LINK_MIN_TIME(m,t) \
	do { \
		if (0 == m.tv_usec) { \
			m.tv_usec = t.tv_usec; \
		} else { \
			m.tv_usec = MIN(m.tv_usec, t.tv_usec); \
		} \
	} while(0)

#define DP_DSM_GET_LINK_MAX_TIME(m,t) \
	do { \
		m.tv_usec = MAX(m.tv_usec, t.tv_usec); \
	} while(0)

typedef struct {
	struct list_head list_head;

	struct mutex lock;
	struct delayed_work imonitor_work;

	// check whether hpd existed or not
	struct delayed_work hpd_work;
	bool hpd_existed;
	unsigned long hpd_jiffies;

	uint32_t report_num[DP_IMONITOR_TYPE_NUM];
	unsigned long report_jiffies;

	// report-skip data of the last day
	bool report_skip_existed;
	uint32_t last_report_num[DP_IMONITOR_TYPE_NUM];
	struct timeval last_link_min_time;
	struct timeval last_link_max_time;
	struct timeval last_same_mode_time;
	struct timeval last_diff_mode_time;
	int last_source_switch_num;
} dp_imonitor_report_info_t;

typedef int (*imonitor_prepare_param_cb_t)(struct imonitor_eventobj *, void *);

typedef struct {
	dp_imonitor_type_t type;
	unsigned int event_id;
	imonitor_prepare_param_cb_t prepare_cb;
} dp_imonitor_event_id_t;

typedef struct {
	dp_dmd_type_t type;
	int error_no;
} dp_dmd_error_no_t;

static int dp_imonitor_prepare_basic_info(struct imonitor_eventobj *obj, void *data);
static int dp_imonitor_prepare_time_info(struct imonitor_eventobj *obj, void *data);
static int dp_imonitor_prepare_extend_info(struct imonitor_eventobj *obj, void *data);
static int dp_imonitor_prepare_hpd_info(struct imonitor_eventobj *obj, void *data);
static int dp_imonitor_prepare_link_training_info(struct imonitor_eventobj *obj, void *data);
static int dp_imonitor_prepare_hotplug_info(struct imonitor_eventobj *obj, void *data);
static int dp_imonitor_prepare_hdcp_info(struct imonitor_eventobj *obj, void *data);

// imonitor no.
#define DSM_DP_BASIC_INFO_NO         (936000102)
#define DSM_DP_TIME_INFO_NO          (936000103)
#define DSM_DP_EXTEND_INFO_NO        (936000104)
#define DSM_DP_HPD_INFO_NO           (936000105)
#define DSM_DP_LINK_TRAINING_INFO_NO (936000106)
#define DSM_DP_HOTPLUG_INFO_NO       (936000107)
#define DSM_DP_HDCP_INFO_NO          (936000108)

static dp_imonitor_event_id_t imonitor_event_id[] = {
	{ DP_IMONITOR_BASIC_INFO,    DSM_DP_BASIC_INFO_NO,         dp_imonitor_prepare_basic_info },
	{ DP_IMONITOR_TIME_INFO,     DSM_DP_TIME_INFO_NO,          dp_imonitor_prepare_time_info },
	{ DP_IMONITOR_EXTEND_INFO,   DSM_DP_EXTEND_INFO_NO,        dp_imonitor_prepare_extend_info },
	{ DP_IMONITOR_HPD,           DSM_DP_HPD_INFO_NO,           dp_imonitor_prepare_hpd_info },
	{ DP_IMONITOR_LINK_TRAINING, DSM_DP_LINK_TRAINING_INFO_NO, dp_imonitor_prepare_link_training_info },
	{ DP_IMONITOR_HOTPLUG,       DSM_DP_HOTPLUG_INFO_NO,       dp_imonitor_prepare_hotplug_info },
	{ DP_IMONITOR_HDCP,          DSM_DP_HDCP_INFO_NO,          dp_imonitor_prepare_hdcp_info },
};

static dp_dmd_error_no_t dmd_error_no[] = {
	// NOTE: 936000100 and 936000101 have been discarded
	// { DP_DMD_LINK_SUCCESS, DSM_DP_BASIC_DISPLAY_TIME_NO },     // 936000101
	// { DP_DMD_LINK_FAILED,  DSM_DP_BASIC_EXTERNAL_DISPLAY_NO }, // 936000100
};

static struct dsm_dev dp_dsm = {
	.name        = "dsm_dp",
	.device_name = NULL,
	.ic_name     = NULL,
	.module_name = NULL,
	.fops        = NULL,
	.buff_size   = DP_DSM_BUF_SIZE,
};

#ifdef DP_DEBUG_ENABLE
static dp_typec_in_out_t g_typec_in_out;
#endif
static dp_dsm_priv_t *g_dp_dsm_priv = NULL;
static struct dsm_client *g_dp_dsm_client = NULL;
static dp_imonitor_report_info_t g_imonitor_report;

#define DP_DSM_BIN_TO_STR_MASK   (0xF)
#define DP_DSM_BIN_TO_STR_SHIFT4 (4)
#define DP_DSM_BIN_TO_STR_SHIFT8 (8)
#define DP_DSM_BIN_TO_STR_TIMES  (DP_DSM_BIN_TO_STR_SHIFT8 / DP_DSM_BIN_TO_STR_SHIFT4)

static const unsigned char data_bin2ascii[17] = "0123456789ABCDEF";
#define conv_bin2ascii(a) (data_bin2ascii[(a) & DP_DSM_BIN_TO_STR_MASK])

#define DP_DSM_EDID_HEADER_SIZE      (8)
#define DP_DSM_EDID_EXT_HEADER       (0x02)
#define DP_DSM_EDID_EXT_BLOCKS_INDEX (126)
#define DP_DSM_EDID_EXT_BLOCKS_MAX   (3)
#define DP_DSM_PARAM_NAME_MAX        (16)

static const uint8_t g_edid_v1_header[DP_DSM_EDID_HEADER_SIZE] = {0x00, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0x00};

// with params
#define DP_DSM_CHECK_HOTPLUG_PTR_P(p, m) \
	do { \
		if ((NULL == p) || (NULL == p->cur_hotplug_info) || (NULL == m)) { \
			hwlog_err("%s: priv, hotplug or param is NULL!\n", __func__); \
			return; \
		} \
		hotplug = p->cur_hotplug_info; \
	} while(0)

// with result
#define DP_DSM_CHECK_HOTPLUG_PTR_R(p, m) \
	do { \
		if ((NULL == p) || (NULL == p->cur_hotplug_info) || (NULL == m)) { \
			hwlog_err("%s: priv, hotplug or param is NULL!\n", __func__); \
			return -EINVAL; \
		} \
		hotplug = p->cur_hotplug_info; \
	} while(0)

#define DP_DSM_FREE_HOTPLUG_PTR(p) \
	do { \
		if (p->cur_hotplug_info != NULL) { \
			kfree(p->cur_hotplug_info); \
			p->cur_hotplug_info = NULL; \
		} \
	} while(0)

#define DP_DSM_ADD_TO_IMONITOR_LIST(p, f, t) \
	do { \
		if (p->f) { \
			p->f = false; \
			dp_add_info_to_imonitor_list(t, p); \
		} \
	} while(0)

#define DP_DSM_CHECK_LINK_STATE(p, s) (p->hotplug_state & (1 << s))
#define DP_DSM_REPORT_LINK_STATE(p, s) \
	do { \
		dp_link_state_event(s); \
		p->last_hotplug_state |= (1 << s); \
	} while(0)

// param time:
// true:  only time
// false: date and time
static int dp_timeval_to_str(struct timeval time, const char *comment, char *str, int size, bool only_time)
{
	struct rtc_time tm;
	int ret = 0;

	if ((NULL == comment) || (NULL == str)) {
		hwlog_err("%s: comment or str is NULL!!!\n", __func__);
		return -EINVAL;
	}
	memset(str, 0, size);

	time.tv_sec -= sys_tz.tz_minuteswest * DP_DSM_TIME_SECOND_OF_MINUTE;
	rtc_time_to_tm(time.tv_sec, &tm);

	if (only_time) {
		ret = snprintf(str, size, "%02d:%02d:%02d", tm.tm_hour, tm.tm_min, tm.tm_sec);
	} else {
		ret = snprintf(str, size, "%04d-%02d-%02d %02d:%02d:%02d",
			tm.tm_year + DP_DSM_TIME_YEAR_OFFSET, tm.tm_mon + DP_DSM_TIME_MONTH_OFFSET,
			tm.tm_mday, tm.tm_hour, tm.tm_min, tm.tm_sec);
	}

#ifdef DP_DSM_DEBUG
	if (ret < 0) {
		hwlog_info("%s: %s failed %d.\n", __func__, comment, ret);
	} else {
		hwlog_info("%s: %s is %s(%d).\n", __func__, comment, str, ret);
	}
#endif

	return 0;
}

#ifdef DP_DEBUG_ENABLE
static int dp_timeval_to_time_str(struct timeval time, const char *comment, char *str, int size)
{
	return dp_timeval_to_str(time, comment, str, size, true);
}
#endif

static int dp_timeval_to_datetime_str(struct timeval time, const char *comment, char *str, int size)
{
	return dp_timeval_to_str(time, comment, str, size, false);
}

#ifdef DP_LINK_TIMEVAL_FORMAT
static int dp_timeval_sec_to_str(unsigned long time, const char *comment, char *str, int size)
{
	struct rtc_time tm;
	int ret = 0;

	if ((NULL == comment) || (NULL == str)) {
		hwlog_err("%s: comment or str is NULL!!!\n", __func__);
		return -EINVAL;
	}
	memset(str, 0, size);

	tm.tm_mday = time / DP_DSM_TIME_SECOND_OF_DAY;
	time -= (unsigned long)tm.tm_mday * DP_DSM_TIME_SECOND_OF_DAY;

	tm.tm_hour = time / DP_DSM_TIME_SECOND_OF_HOUR;
	time -= (unsigned long)tm.tm_hour * DP_DSM_TIME_SECOND_OF_HOUR;

	tm.tm_min = time / DP_DSM_TIME_SECOND_OF_MINUTE;
	tm.tm_sec = time - (unsigned long)tm.tm_min * DP_DSM_TIME_SECOND_OF_MINUTE;

	ret = snprintf(str, size, "%d %02d:%02d:%02d", tm.tm_mday, tm.tm_hour, tm.tm_min, tm.tm_sec);
#ifdef DP_DSM_DEBUG
	if (ret < 0) {
		hwlog_info("%s: %s failed %d.\n", __func__, comment, ret);
	} else {
		hwlog_info("%s: %s is %s(%d).\n", __func__, comment, str, ret);
	}
#endif

	return 0;
}
#endif // DP_LINK_TIMEVAL_FORMAT

static int dp_data_bin_to_str(char *dst, int dst_len, uint8_t *src, int src_len, int shift)
{
	int remaining = src_len;

	if ((NULL == dst) || (NULL == src)) {
		hwlog_err("%s: dst or src is NULL!!!\n", __func__);
		return -EINVAL;
	}

	if (DP_DSM_BIN_TO_STR_SHIFT8 == shift) {
		if (dst_len <= src_len) {
			hwlog_err("%s: invalid params %d<=%d!!!\n", __func__, dst_len, src_len);
			return -EINVAL;
		}
	} else { // DP_DSM_BIN_TO_STR_SHIFT4 == shift
		if (dst_len <= (src_len * DP_DSM_BIN_TO_STR_TIMES)) {
			hwlog_err("%s: invalid params %d<=%d*2!!!\n", __func__, dst_len, src_len);
			return -EINVAL;
		}
	}

	memset(dst, 0, dst_len);
	while (remaining) {
		if (DP_DSM_BIN_TO_STR_SHIFT8 == shift) {
			*(dst++) = conv_bin2ascii(*(src++));
		} else { // DP_DSM_BIN_TO_STR_SHIFT4 == shift
			// high bits
			*(dst++) = conv_bin2ascii(((*src) >> DP_DSM_BIN_TO_STR_SHIFT4) & DP_DSM_BIN_TO_STR_MASK);
			// low bits
			*(dst++) = conv_bin2ascii((*src) & DP_DSM_BIN_TO_STR_MASK);
			src++;
		}

		remaining--;
	}

	return 0;
}

void dp_set_hotplug_state(dp_link_state_t state)
{
	dp_dsm_priv_t *priv = g_dp_dsm_priv;
	int old_state = 0;

	if (NULL == priv) {
		hwlog_err("%s: priv is NULL!!!\n", __func__);
		return;
	}

	if (priv->last_hotplug_state & (1 << state)) {
		hwlog_info("%s: last_hotplug_state=0x%x, state=%d, skip.\n", __func__, priv->last_hotplug_state, state);
		return;
	}

	old_state = priv->hotplug_state;
	priv->hotplug_state |= (1 << state);
	hwlog_info("%s: last_hotplug_state=0x%x, hotplug_state old=0x%x, new=0x%x.\n", __func__,
		priv->last_hotplug_state, old_state, priv->hotplug_state);
}
EXPORT_SYMBOL_GPL(dp_set_hotplug_state);

static void dp_report_hotplug_state(dp_dsm_priv_t *priv)
{
	if (NULL == priv) {
		hwlog_err("%s: priv is NULL!!!\n", __func__);
		return;
	}

	hwlog_info("%s: hotplug_state=0x%x.\n", __func__, priv->hotplug_state);
	if (priv->hotplug_retval < 0) { // hotplug failed
		// for MMIE test
		if (dp_factory_mode_is_enable()) {
			if (DP_DSM_CHECK_LINK_STATE(priv, DP_LINK_STATE_EDID_FAILED)) { // 1024*768
				DP_DSM_REPORT_LINK_STATE(priv, DP_LINK_STATE_EDID_FAILED);
				goto ret_out;
			} else if (DP_DSM_CHECK_LINK_STATE(priv, DP_LINK_STATE_SAFE_MODE)
				&& (priv->hotplug_width == DP_SAFE_MODE_DISPLAY_WIDTH)
				&& (priv->hotplug_high == DP_SAFE_MODE_DISPLAY_HIGH)) { // safe mode, 640*480
				DP_DSM_REPORT_LINK_STATE(priv, DP_LINK_STATE_SAFE_MODE);
				goto ret_out;
			} else if (DP_DSM_CHECK_LINK_STATE(priv, DP_LINK_STATE_INVALID_COMBINATIONS)) {
				DP_DSM_REPORT_LINK_STATE(priv, DP_LINK_STATE_INVALID_COMBINATIONS);
				goto ret_out;
			} else if (DP_DSM_CHECK_LINK_STATE(priv, DP_LINK_STATE_LINK_REDUCE_RATE)) {
				DP_DSM_REPORT_LINK_STATE(priv, DP_LINK_STATE_LINK_REDUCE_RATE);
				goto ret_out;
			}
		}

		if (DP_DSM_CHECK_LINK_STATE(priv, DP_LINK_STATE_LINK_FAILED)) {
			DP_DSM_REPORT_LINK_STATE(priv, DP_LINK_STATE_LINK_FAILED);
		} else if (DP_DSM_CHECK_LINK_STATE(priv, DP_LINK_STATE_AUX_FAILED)) {
			DP_DSM_REPORT_LINK_STATE(priv, DP_LINK_STATE_AUX_FAILED);
		} else {
			hwlog_info("%s: hotplug failed, hotplug_state=0x%x, skip!\n", __func__, priv->hotplug_state);
		}
	} else { // hotplug success
		if (DP_DSM_CHECK_LINK_STATE(priv, DP_LINK_STATE_EDID_FAILED)) { // 1024*768
			DP_DSM_REPORT_LINK_STATE(priv, DP_LINK_STATE_EDID_FAILED);
		} else if (DP_DSM_CHECK_LINK_STATE(priv, DP_LINK_STATE_SAFE_MODE)
			&& (priv->hotplug_width == DP_SAFE_MODE_DISPLAY_WIDTH)
			&& (priv->hotplug_high == DP_SAFE_MODE_DISPLAY_HIGH)) { // safe mode, 640*480
			DP_DSM_REPORT_LINK_STATE(priv, DP_LINK_STATE_SAFE_MODE);
		} else {
			hwlog_info("%s: hotplug success, hotplug_state=0x%x, skip!\n", __func__, priv->hotplug_state);
		}
	}

ret_out:
	priv->hotplug_state = 0;
	dp_factory_set_link_event_no(priv->last_hotplug_state, false, NULL, priv->hotplug_retval);
}

static int dp_add_info_to_imonitor_list(dp_imonitor_type_t type, dp_dsm_priv_t *data)
{
	dp_dsm_priv_t *priv = NULL;
	dp_hotplug_info_t *hotplug = NULL;
	int ret = 0;

	if (NULL == data) {
		hwlog_err("%s: data is NULL!!!\n", __func__);
		return -EINVAL;
	}

	mutex_lock(&(g_imonitor_report.lock));
	priv = (dp_dsm_priv_t *)kzalloc(sizeof(dp_dsm_priv_t), GFP_KERNEL);
	hotplug = (dp_hotplug_info_t *)kzalloc(sizeof(dp_hotplug_info_t), GFP_KERNEL);
	if ((NULL == priv) || (NULL == hotplug)) {
		hwlog_err("%s: priv or hotplug is NULL!!!\n", __func__);
		ret = -ENOMEM;
		goto err_out;
	}

	memcpy(priv, data, sizeof(dp_dsm_priv_t));
	switch (type) {
	case DP_IMONITOR_BASIC_INFO:
	case DP_IMONITOR_EXTEND_INFO:
	case DP_IMONITOR_LINK_TRAINING:
		if (data->cur_hotplug_info != NULL) {
			memcpy(hotplug, data->cur_hotplug_info, sizeof(dp_hotplug_info_t));
		} else {
			hwlog_err("%s: cur_hotplug_info is NULL!!!\n", __func__);
			ret = -EFAULT;
			goto err_out;
		}
		break;
	case DP_IMONITOR_TIME_INFO:
	case DP_IMONITOR_HPD:
	case DP_IMONITOR_HOTPLUG:
	case DP_IMONITOR_HDCP:
	default:
		kfree(hotplug);
		hotplug = NULL;
		break;
	}

	priv->type = type;
	priv->cur_hotplug_info = hotplug;
	list_add_tail(&(priv->list), &(g_imonitor_report.list_head));

err_out:
	if (ret < 0) {
		if (priv != NULL) {
			kfree(priv);
		}

		if (hotplug != NULL) {
			kfree(hotplug);
		}
	}
	mutex_unlock(&(g_imonitor_report.lock));

	return ret;
}

static void dp_imonitor_report_info_work(struct work_struct *work)
{
	dp_imonitor_report_info_t *report = NULL;
	struct list_head *pos = NULL, *q = NULL;
	dp_dsm_priv_t *priv = NULL;
	int list_count = 0;

	if (NULL == work) {
		hwlog_err("%s: invalid argument!!!\n", __func__);
		return;
	}

	report = container_of(work, dp_imonitor_report_info_t, imonitor_work.work);

	mutex_lock(&(g_imonitor_report.lock));
	if (!time_is_after_jiffies(g_imonitor_report.report_jiffies)) {
		memset(g_imonitor_report.report_num, 0, sizeof(uint32_t) * DP_IMONITOR_TYPE_NUM);
		g_imonitor_report.report_jiffies = jiffies + msecs_to_jiffies(DP_DSM_REPORT_TIME_INTERVAL);
	}

	list_for_each_safe(pos, q, &(report->list_head)) {
		priv = list_entry(pos, dp_dsm_priv_t, list);
		list_del(pos);
		list_count++;

		if (g_imonitor_report.report_num[priv->type] < DP_DSM_REPORT_NUM_IN_ONE_DAY) {
			hwlog_info("%s: imonitor report %d(%d/%d).\n", __func__, priv->type,
				g_imonitor_report.report_num[priv->type] + 1, DP_DSM_REPORT_NUM_IN_ONE_DAY);
			dp_imonitor_report(priv->type, priv);
		} else {
			hwlog_info("%s: imonitor report %d(%d/%d), skip!\n", __func__, priv->type,
				g_imonitor_report.report_num[priv->type] + 1, DP_DSM_REPORT_NUM_IN_ONE_DAY);

			// record the key information of the last day
			g_imonitor_report.report_skip_existed = true;
			g_imonitor_report.last_report_num[priv->type]++;

			if (DP_IMONITOR_TIME_INFO == priv->type) {
				DP_DSM_GET_LINK_MIN_TIME(g_imonitor_report.last_link_min_time, priv->link_min_time);
				DP_DSM_GET_LINK_MAX_TIME(g_imonitor_report.last_link_max_time, priv->link_max_time);

				g_imonitor_report.last_same_mode_time.tv_sec += priv->same_mode_time.tv_sec;
				g_imonitor_report.last_diff_mode_time.tv_sec += priv->diff_mode_time.tv_sec;

				g_imonitor_report.last_source_switch_num += priv->source_switch_num;
			}
		}
		g_imonitor_report.report_num[priv->type]++;

		DP_DSM_FREE_HOTPLUG_PTR(priv);
		kfree(priv);
	}
	hwlog_info("%s: imonitor report success(%d).\n", __func__, list_count);
	mutex_unlock(&(g_imonitor_report.lock));
}

static void dp_hpd_check_work(struct work_struct *work)
{
	dp_imonitor_report_info_t *report = NULL;
	dp_dsm_priv_t *priv = g_dp_dsm_priv;

	if ((NULL == work) || (NULL == priv)) {
		hwlog_err("%s: invalid argument!!!\n", __func__);
		return;
	}

	report = container_of(work, dp_imonitor_report_info_t, hpd_work.work);
	if (!report->hpd_existed) {
		hwlog_err("%s: hpd not existed!\n", __func__);
		priv->hotplug_retval = -DP_DSM_ERRNO_HPD_NOT_EXISTED;
		DP_DSM_REPORT_LINK_STATE(priv, DP_LINK_STATE_HPD_NOT_EXISTED);
		dp_factory_set_link_event_no(priv->last_hotplug_state, false, NULL, priv->hotplug_retval);
	}
}

static void dp_clear_timing_info(dp_dsm_priv_t *priv)
{
	struct list_head *pos = NULL, *q = NULL;
	dp_timing_info_t *timing = NULL;

	if (NULL == priv) {
		hwlog_err("%s: priv is NULL!!!\n", __func__);
		return;
	}

	list_for_each_safe(pos, q, &(priv->timing_list_head)) {
		timing = list_entry(pos, dp_timing_info_t, list);
		list_del(pos);
		kfree(timing);
	}
}

static void dp_clear_priv_data(dp_dsm_priv_t *priv)
{
	struct list_head *pos = NULL, *q = NULL;
	dp_connected_event_t *event = NULL;
	dp_hotplug_info_t *hotplug = NULL;

	if (NULL == priv) {
		hwlog_err("%s: priv is NULL!!!\n", __func__);
		return;
	}

	list_for_each_safe(pos, q, &(priv->event_list_head)) {
		event = list_entry(pos, dp_connected_event_t, list);
		list_del(pos);
		kfree(event);
	}

	list_for_each_safe(pos, q, &(priv->hotplug_list_head)) {
		hotplug = list_entry(pos, dp_hotplug_info_t, list);
		list_del(pos);
		kfree(hotplug);
	}
	dp_clear_timing_info(priv);

#ifndef DP_DEBUG_ENABLE
	DP_DSM_FREE_HOTPLUG_PTR(priv);
#endif
}

static void dp_reinit_priv_data(dp_dsm_priv_t *priv)
{
	if (NULL == priv) {
		hwlog_err("%s: priv is NULL!!!\n", __func__);
		return;
	}

	memset(priv, 0, sizeof(dp_dsm_priv_t));
	priv->rcf_basic_info_of_same_mode = true;
	priv->rcf_basic_info_of_diff_mode = true;
	priv->rcf_extend_info = true;
	priv->rcf_link_training_info = true;
	priv->rcf_link_retraining_info = true;
	priv->rcf_hotplug_info = true;
	priv->rcf_hdcp_info = true;

	INIT_LIST_HEAD(&(priv->event_list_head));
	INIT_LIST_HEAD(&(priv->hotplug_list_head));
	INIT_LIST_HEAD(&(priv->timing_list_head));
}

static void dp_add_hotplug_info_to_list(dp_dsm_priv_t *priv, bool stop)
{
	dp_hotplug_info_t *hotplug = NULL;

	if ((NULL == priv) || (NULL == priv->cur_hotplug_info) || !priv->need_add_hotplug_to_list) {
		//hwlog_err("%s: priv or hotplug is NULL!!!\n", __func__);
		return;
	}
	hotplug = priv->cur_hotplug_info;
	priv->need_add_hotplug_to_list = false;

	// stop time
	if (!stop) {
		//hwlog_info("%s: no hotunplug!!!\n", __func__);
		hotplug->no_hotunplug = true;
		do_gettimeofday(&(hotplug->time[DP_PARAM_TIME_STOP - DP_PARAM_TIME]));
	}

	hotplug->hotplug_retval = priv->hotplug_retval;
#ifdef DP_DEBUG_ENABLE
	list_add_tail(&(hotplug->list), &(priv->hotplug_list_head));
#endif

	if (0 == priv->hotplug_retval) {
		if (stop) {
			struct timeval sec;

			// same or diff mode time
			sec.tv_sec = hotplug->time[DP_PARAM_TIME_STOP - DP_PARAM_TIME].tv_sec
				- hotplug->time[DP_PARAM_TIME_LINK_SUCC - DP_PARAM_TIME].tv_sec;
			if (hotplug->source_mode) {
				priv->same_mode_time.tv_sec += sec.tv_sec;
			} else {
				priv->diff_mode_time.tv_sec += sec.tv_sec;
			}

			// link succ time
			sec.tv_sec = hotplug->time[DP_PARAM_TIME_LINK_SUCC - DP_PARAM_TIME].tv_sec
				- hotplug->time[DP_PARAM_TIME_START - DP_PARAM_TIME].tv_sec;
			sec.tv_usec = hotplug->time[DP_PARAM_TIME_LINK_SUCC - DP_PARAM_TIME].tv_usec
				- hotplug->time[DP_PARAM_TIME_START - DP_PARAM_TIME].tv_usec;
			sec.tv_usec = (sec.tv_sec * DP_DSM_TIME_MS_OF_SEC) + (sec.tv_usec / DP_DSM_TIME_US_OF_MS);

			DP_DSM_GET_LINK_MIN_TIME(priv->link_min_time, sec);
			DP_DSM_GET_LINK_MAX_TIME(priv->link_max_time, sec);
		}

		if (0 == hotplug->read_edid_retval) {
#ifdef DP_DEBUG_ENABLE
			// invalid edid
			if ((hotplug->edid_check.u32.header_flag != EDID_CHECK_SUCCESS)
				|| (hotplug->edid_check.u32.checksum_flag != EDID_CHECK_SUCCESS)
				|| (hotplug->edid_check.u32.ext_blocks > DP_DSM_EDID_EXT_BLOCKS_MAX)) {
				dp_add_info_to_imonitor_list(DP_IMONITOR_EXTEND_INFO, priv);
			} else { // valid edid
#endif
				DP_DSM_ADD_TO_IMONITOR_LIST(priv, rcf_extend_info, DP_IMONITOR_EXTEND_INFO);
#ifdef DP_DEBUG_ENABLE
			}
#endif
		}
	}
}

static bool dp_connected_by_typec(uint8_t type)
{
	if ((TCPC_DP == type) || (TCPC_USB31_AND_DP_2LINE == type)) {
		return true;
	}

	return false;
}

#ifdef DP_DEBUG_ENABLE
static void dp_set_typec_in_out_time(dp_dsm_priv_t *priv, bool in)
{
	int index = 0;

	if (NULL == priv) {
		hwlog_err("%s: priv is NULL!!!\n", __func__);
		return;
	}

	index = g_typec_in_out.in_out_num % DP_DSM_TYPEC_IN_OUT_NUM;
	if (in) {
		do_gettimeofday(&(priv->tpyec_in_time));
		g_typec_in_out.in_time[index] = priv->tpyec_in_time;
	} else { // out
		do_gettimeofday(&(priv->tpyec_out_time));
		g_typec_in_out.out_time[index] = priv->tpyec_out_time;

		g_typec_in_out.in_out_num++;
	}
}
#endif

static void dp_typec_cable_in(dp_dsm_priv_t *priv, uint8_t mode_type)
{
	if (NULL == priv) {
		hwlog_err("%s: priv is NULL!!!\n", __func__);
		return;
	}

	hwlog_info("typec cable in.\n");
	dp_link_state_event(DP_LINK_STATE_CABLE_IN);
	dp_factory_set_link_event_no(0, true, NULL, 0);
	dp_clear_priv_data(priv);
	dp_reinit_priv_data(priv);

#ifdef DP_DEBUG_ENABLE
	dp_set_typec_in_out_time(priv, true);
#else
	do_gettimeofday(&(priv->tpyec_in_time));
#endif
	priv->tpyec_cable_type = mode_type;

	// delayed work to check whether hpd existed or not
	if (dp_factory_mode_is_enable()) {
		if ((0 == g_imonitor_report.hpd_jiffies) || !time_is_after_jiffies(g_imonitor_report.hpd_jiffies)) {
			cancel_delayed_work_sync(&(g_imonitor_report.hpd_work));

			g_imonitor_report.hpd_jiffies = 0;
			g_imonitor_report.hpd_existed = false;
			schedule_delayed_work(&(g_imonitor_report.hpd_work), msecs_to_jiffies(DP_DSM_HPD_DELAYED_TIME));
		}
	}
}

static void dp_typec_cable_out(dp_dsm_priv_t *priv)
{
	bool hpd_report = false;

	if (NULL == priv) {
		hwlog_err("%s: priv is NULL!!!\n", __func__);
		return;
	}

	hwlog_info("typec cable out.\n");
	dp_link_state_event(DP_LINK_STATE_CABLE_OUT);
	if (dp_factory_mode_is_enable()) {
		cancel_delayed_work_sync(&(g_imonitor_report.hpd_work));
	}

#ifdef DP_DSM_DEBUG
	hwlog_info("%s: first_dev_type=%u\n",  __func__, priv->tca_dev_type[0]);
	hwlog_info("%s: second_dev_type=%u\n", __func__, priv->tca_dev_type[1]);
#endif
#ifdef DP_DEBUG_ENABLE
	dp_set_typec_in_out_time(priv, false);
#else
	do_gettimeofday(&(priv->tpyec_out_time));
#endif

	// if hotunplug not existed
	dp_add_hotplug_info_to_list(priv, false);

	// dp in event(>1) after typec cable in
	if (priv->dp_in_num > 1) {
		int delta = 0;

		if (priv->source_switch_num > 0) {
			delta = (priv->dp_in_num > priv->source_switch_num) ? (priv->dp_in_num - priv->source_switch_num) : 0;
		} else {
			delta = priv->dp_in_num;
		}

		if (delta > 1) {
			hpd_report = true;
			dp_add_info_to_imonitor_list(DP_IMONITOR_HPD, priv);
		}
	}

	// dp time info report
	if (!hpd_report && (0 == priv->hotplug_retval)
		&& ((priv->same_mode_time.tv_sec > 0) || (priv->diff_mode_time.tv_sec > 0))) {
		dp_add_info_to_imonitor_list(DP_IMONITOR_TIME_INFO, priv);
	}

	// delayed work to avoid report info in irq handler
	cancel_delayed_work_sync(&(g_imonitor_report.imonitor_work));
	schedule_delayed_work(&(g_imonitor_report.imonitor_work), msecs_to_jiffies(DP_DSM_IMONITOR_DELAYED_TIME));
}

static bool dp_is_irq_hpd(uint8_t type)
{
	if ((TCA_IRQ_HPD_OUT == type) || (TCA_IRQ_HPD_IN == type)) {
		return true;
	}

	return false;
}

static void dp_set_first_or_second_dev_type(dp_dsm_priv_t *priv, uint8_t dev_type)
{
	int index = 0;

	if (NULL == priv) {
		hwlog_err("%s: priv is NULL!!!\n", __func__);
		return;
	}

	index = priv->irq_hpd_num % DP_DSM_TCA_DEV_TYPE_NUM;
	priv->tca_dev_type[index] = dev_type;

	priv->irq_hpd_num++;
}

static void dp_set_irq_info(dp_dsm_priv_t *priv, uint8_t irq_type, uint8_t dev_type)
{
	if (NULL == priv) {
		hwlog_err("%s: priv is NULL!!!\n", __func__);
		return;
	}

	priv->event_num++;
	if (dp_is_irq_hpd(irq_type)) {
		if (TCA_DP_IN == dev_type) { // hotplug
			priv->dp_in_num++;
			// report hpd repeatedly from PD module
			// 1. only IN event: reached six times, and then report
			// 2. IN/OUT event: the total number reached six times
			priv->hpd_repeated_num++;
			if (priv->hpd_repeated_num >= DP_HPD_REPEATED_THRESHOLD) {
				if (!dp_factory_mode_is_enable()) {
					dp_link_state_event(DP_LINK_STATE_MULTI_HPD);
				}
				priv->hpd_repeated_num = 0;
			}

			dp_set_first_or_second_dev_type(priv, dev_type);
		} else if (TCA_DP_OUT == dev_type) { // hotunplug
			priv->dp_out_num++;
			// If the first time is pulled out, not counting
			if (priv->hpd_repeated_num > 0) {
				priv->hpd_repeated_num++;
			}
			dp_set_first_or_second_dev_type(priv, dev_type);
		}
	} else {
		priv->irq_short_num++;
	}
}

void dp_imonitor_set_pd_event(uint8_t irq_type, uint8_t cur_mode, uint8_t mode_type, uint8_t dev_type, uint8_t typec_orien)
{
	dp_dsm_priv_t *priv = g_dp_dsm_priv;
#ifdef DP_DEBUG_ENABLE
	dp_connected_event_t *event = NULL;
#endif

	// not dp cable in
	if (!dp_connected_by_typec(cur_mode) && !dp_connected_by_typec(mode_type)) {
		return;
	}

#ifdef DP_DSM_DEBUG
	hwlog_info("%s +\n", __func__);
#endif

	if (NULL == priv) {
		hwlog_err("%s: priv is NULL!!!\n", __func__);
		return;
	}

	// dp cable in, start
	if (!dp_connected_by_typec(cur_mode) && dp_connected_by_typec(mode_type)) {
		dp_typec_cable_in(priv, mode_type);
	} else {
		g_imonitor_report.hpd_existed = true;
	}
	dp_set_irq_info(priv, irq_type, dev_type);

#ifdef DP_DEBUG_ENABLE
	event = (dp_connected_event_t *)kzalloc(sizeof(dp_connected_event_t), GFP_KERNEL);
	if (NULL == event) {
		hwlog_err("%s: kzalloc event failed!!!\n", __func__);
		return;
	}

	do_gettimeofday(&(event->time));
	event->irq_type    = (TCA_IRQ_TYPE_E)irq_type;
	event->cur_mode    = (TCPC_MUX_CTRL_TYPE)cur_mode;
	event->mode_type   = (TCPC_MUX_CTRL_TYPE)mode_type;
	event->dev_type    = (TCA_DEV_TYPE_E)dev_type;
	event->typec_orien = (TYPEC_PLUG_ORIEN_E)typec_orien;

	list_add_tail(&(event->list), &(priv->event_list_head));
#endif

	// dp cable out, stop
	if (dp_connected_by_typec(cur_mode) && !dp_connected_by_typec(mode_type)) {
		dp_typec_cable_out(priv);
	}

#ifdef DP_DSM_DEBUG
	hwlog_info("%s -\n", __func__);
#endif
}
EXPORT_SYMBOL_GPL(dp_imonitor_set_pd_event);

static void dp_set_edid_version(dp_hotplug_info_t *hotplug, uint8_t *edid)
{
	if ((NULL == hotplug) || (NULL == edid)) {
		hwlog_err("%s: hotplug or edid is NULL!!!\n", __func__);
		return;
	}

	hotplug->edid_version = DP_DSM_EDID_VERSION_MAKE(
		edid[DP_DSM_EDID_VERSION_OFFSET], edid[DP_DSM_EDID_REVISION_OFFSET]);
#ifdef DP_DSM_DEBUG
	hwlog_info("%s: edid_version is %x.%x\n", __func__,
		edid[DP_DSM_EDID_VERSION_OFFSET], edid[DP_DSM_EDID_REVISION_OFFSET]);
#endif
}

static void dp_set_manufacturer_id(dp_hotplug_info_t *hotplug, uint8_t *edid)
{
	uint16_t id = 0;
	int index = 0; // letter index, 1 ~ 26
	int i = 0;

	if ((NULL == hotplug) || (NULL == edid)) {
		hwlog_err("%s: hotplug or edid is NULL!!!\n", __func__);
		return;
	}

	// byte 8 and byte 9
	id = DP_DSM_MID_LETTER(edid[DP_DSM_MID_OFFSET], edid[DP_DSM_MID_OFFSET + 1]);
	if (id == 0) {
		hwlog_err("%s: id is 0, invalid params!!!\n", __func__);
		return;
	}

	// If index = 0, the letter is '@'('A' + (-1)).
	for (i = 0; i < DP_DSM_MID_LETTER_NUM; i++) {
		index  = id >> ((DP_DSM_MID_LETTER_NUM - i - 1) * DP_DSM_MID_LETTER_OFFSET);
		index &= DP_DSM_MID_LETTER_MASK;
		index -= 1; // letter start 1, 00001=A

		hotplug->mid[i] = 'A' + index;
	}
	hotplug->mid[i] = '\0';
#ifdef DP_DEBUG_ENABLE
	hwlog_info("manufacturer id is %s\n", hotplug->mid);
#endif
}

static void dp_set_monitor_info(dp_hotplug_info_t *hotplug, uint8_t *edid)
{
	unsigned char *block = NULL;
	int i = 0;
	int j = 0;

	if ((NULL == hotplug) || (NULL == edid)) {
		hwlog_err("%s: hotplug or edid is NULL!!!\n", __func__);
		return;
	}

	block = edid + DP_DSM_EDID_DESC_OFFSET;
	for (i = 0; i < DP_DSM_EDID_DESC_NUM; i++, block += DP_DSM_EDID_DESC_SIZE) {
		if (block[DP_DSM_EDID_DESC_MASK_OFFSET] == DP_DSM_EDID_DESC_MONITOR_MASK) {
			// 00 00 00 fc 00 53 6b 79 20 4c 43 44 20 2a 2a 2a 0a 20
			for (j = DP_DSM_EDID_DESC_MONITOR_OFFSET; j < DP_DSM_EDID_DESC_SIZE; j++) {
				if (block[j] == DP_DSM_EDID_DESC_MONITOR_TAIL) {
					break;
				}

				hotplug->monitor[j - DP_DSM_EDID_DESC_MONITOR_OFFSET] = block[j];
			}
			hotplug->monitor[j - DP_DSM_EDID_DESC_MONITOR_OFFSET] = '\0';
			break;
		}
	}

#ifdef DP_DEBUG_ENABLE
	hwlog_info("monitor name is %s\n", hotplug->monitor);
#endif
}

static int dp_check_edid(dp_hotplug_info_t *hotplug, uint8_t *edid, int index)
{
	bool verify_header = true;
	uint32_t checksum = 0;
	int i = 0;

	if ((NULL == hotplug) || (NULL == edid)) {
		hwlog_err("%s: hotplug or edid is NULL!!!\n", __func__);
		return -EINVAL;
	}

	// verify (checksum == 0)
	for (i = 0; i < DP_DSM_EDID_BLOCK_SIZE; i++) {
		checksum += edid[i];
	}

	if (checksum & 0xFF) {
		hotplug->edid_check.u8.checksum_num[index]++;
		hotplug->edid_check.u8.checksum_flag[index] = EDID_CHECK_FAILED;
	} else {
		hotplug->edid_check.u8.checksum_flag[index] = EDID_CHECK_SUCCESS;
	}

	if (0 == index) {
		// verify header
		for (i = 0; i < DP_DSM_EDID_HEADER_SIZE; i++) {
			if (edid[i] != g_edid_v1_header[i]) {
				hotplug->edid_check.u8.header_num[index]++;
                verify_header = false;
				break;
			}
		}

		hotplug->edid_check.u8.ext_blocks = edid[DP_DSM_EDID_EXT_BLOCKS_INDEX];
		if (hotplug->edid_check.u8.ext_blocks > DP_DSM_EDID_EXT_BLOCKS_MAX) {
			hotplug->edid_check.u8.ext_blocks_num++;
		}
	} else {
		// Check Extension Tag
		// Header Tag stored in the first uint8_t
		if (edid[0] != DP_DSM_EDID_EXT_HEADER) {
			hotplug->edid_check.u8.header_num[index]++;
			verify_header = false;
		}
	}

	if (verify_header) {
		hotplug->edid_check.u8.header_flag[index] = EDID_CHECK_SUCCESS;
	} else {
		hotplug->edid_check.u8.header_flag[index] = EDID_CHECK_FAILED;
	}

#ifdef DP_DSM_DEBUG
	hwlog_info("%s: %d header_flag=0x%08x, checksum_flag=0x%08x\n", __func__,
		index, hotplug->edid_check.u32.header_flag, hotplug->edid_check.u32.checksum_flag);
#endif
	return 0;
}

static void dp_set_edid_block(dp_dsm_priv_t *priv, dp_imonitor_param_t param, uint8_t *data)
{
	dp_hotplug_info_t *hotplug = NULL;
	uint8_t *edid = NULL;
	int index = (int)(param - DP_PARAM_EDID);
#ifdef DP_DSM_DEBUG
	int i = 0;
#endif

	DP_DSM_CHECK_HOTPLUG_PTR_P(priv, data);
	if ((index < 0) || (index >= (int)DP_PARAM_EDID_NUM)) {
		hwlog_err("%s: invalid index %d!!!\n", __func__, index);
		return;
	}

	edid = hotplug->edid[index];
	dp_check_edid(hotplug, data, index);

	// edid block0
	if (0 == index) {
		hotplug->current_edid_num = 0;
		dp_clear_timing_info(priv);
#ifdef DP_DEBUG_ENABLE
		if (0 == dp_debug_get_lanes_rate_force(&priv->lanes_force, &priv->rate_force)) {
			priv->need_lanes_force = true;
			priv->need_rate_force  = true;
		}

		if (0 == dp_debug_get_resolution_force(&priv->user_mode, &priv->user_format)) {
			priv->need_resolution_force = true;
		}
#endif

		// parse edid: version, mid, monitor
		dp_set_edid_version(hotplug, data);
		dp_set_manufacturer_id(hotplug, data);
		dp_set_monitor_info(hotplug, data);
	}

	memcpy(edid, data, DP_DSM_EDID_BLOCK_SIZE);
	hotplug->current_edid_num++;
#ifdef DP_DEBUG_ENABLE
	if (priv->need_resolution_force) {
		data[DP_DSM_EDID_BLOCK_SIZE - 1] = ~data[DP_DSM_EDID_BLOCK_SIZE - 1];
	}
#endif

#ifdef DP_DSM_DEBUG
	for (i = 0; i < DP_DSM_EDID_BLOCK_SIZE; i += 16) {
		hwlog_info("EDID%d[%02x]:%02x %02x %02x %02x %02x %02x %02x %02x  %02x %02x %02x %02x %02x %02x %02x %02x\n",
			index, i,
			edid[i],	  edid[i + 1],	edid[i + 2],  edid[i + 3],
			edid[i + 4],  edid[i + 5],	edid[i + 6],  edid[i + 7],
			edid[i + 8],  edid[i + 9],	edid[i + 10], edid[i + 11],
			edid[i + 12], edid[i + 13], edid[i + 14], edid[i + 15]);
	}
#endif
}

static void dp_set_dpcd_rx_caps(dp_dsm_priv_t *priv, uint8_t *rx_caps)
{
	dp_hotplug_info_t *hotplug = NULL;
#ifdef DP_DSM_DEBUG
	int i = 0;
#endif

	DP_DSM_CHECK_HOTPLUG_PTR_P(priv, rx_caps);
	hotplug->dpcd_revision = rx_caps[0];

#ifdef DP_DSM_DEBUG
	hwlog_info("%s: dpcd_revision is %x.%x\n", __func__,
		(hotplug->dpcd_revision >> DP_DSM_DPCD_REVISION_OFFSET) & DP_DSM_DPCD_REVISION_MASK,
		hotplug->dpcd_revision & DP_DSM_DPCD_REVISION_MASK);

	for (i = 0; i < DP_DSM_DPCD_RX_CAPS_SIZE; i += 16) {
		hwlog_info("DPCD[%02x]:%02x %02x %02x %02x %02x %02x %02x %02x  %02x %02x %02x %02x %02x %02x %02x %02x\n", i,
			rx_caps[i],	     rx_caps[i + 1], rx_caps[i + 2],   rx_caps[i + 3],
			rx_caps[i + 4],  rx_caps[i + 5], rx_caps[i + 6],   rx_caps[i + 7],
			rx_caps[i + 8],  rx_caps[i + 9], rx_caps[i + 10],  rx_caps[i + 11],
			rx_caps[i + 12], rx_caps[i + 13], rx_caps[i + 14], rx_caps[i + 15]);
	}
#endif
}

static void dp_set_link_tu(dp_hotplug_info_t *hotplug, int tu)
{
	if (NULL == hotplug) {
		hwlog_err("%s: hotplug is NULL!!!\n", __func__);
		return;
	}

	if (tu >= DP_DSM_TU_FAIL) {
		hotplug->tu_fail = tu;
	} else {
		hotplug->tu = tu;
	}
}

static void dp_set_irq_vector(dp_dsm_priv_t *priv, uint8_t *vector)
{
	int index = 0;

	if ((NULL == priv) || (NULL == vector)) {
		hwlog_err("%s: priv or vector is NULL!!!\n", __func__);
		return;
	}

	index = priv->irq_vector_num % DP_DSM_IRQ_VECTOR_NUM;
	priv->irq_vector[index] = (*vector);

	priv->irq_vector_num++;
}

static void dp_set_source_switch(dp_dsm_priv_t *priv, int *source)
{
	int index = 0;

	if ((NULL == priv) || (NULL == source)) {
		hwlog_err("%s: priv or source is NULL!!!\n", __func__);
		return;
	}

	index = priv->source_switch_num % DP_DSM_SOURCE_SWITCH_NUM;
	priv->source_switch[index] = (uint8_t)(*source);

	priv->source_switch_num++;
	priv->hpd_repeated_num = 0;
}

static void dp_set_param_time_info(dp_dsm_priv_t *priv, dp_imonitor_param_t param)
{
	int index = (int)(param - DP_PARAM_TIME);

	if (NULL == priv) {
		hwlog_err("%s: priv is NULL!!!\n", __func__);
		return;
	}

	if ((index < 0) || (index >= (int)DP_PARAM_TIME_NUM)) {
		hwlog_err("%s: invalid index %d!!!\n", __func__, index);
		return;
	}

	// hotplug, dp start
	if (DP_PARAM_TIME_START == param) {
		dp_hotplug_info_t *hotplug = NULL;

		// if hotunplug not existed
		dp_add_hotplug_info_to_list(priv, false);

#ifdef DP_DEBUG_ENABLE
		hotplug = (dp_hotplug_info_t *)kzalloc(sizeof(dp_hotplug_info_t), GFP_KERNEL);
#else
		if (NULL == priv->cur_hotplug_info) {
			hotplug = (dp_hotplug_info_t *)kzalloc(sizeof(dp_hotplug_info_t), GFP_KERNEL);
		} else {
			hotplug = priv->cur_hotplug_info;
			memset(hotplug, 0, sizeof(dp_hotplug_info_t));
		}
#endif
		if (NULL == hotplug) {
			hwlog_err("%s: kzalloc hotplug failed!!!\n", __func__);
			return;
		}

		// init data
		priv->cur_source_mode = false; // true: same source, false: diff source
		priv->dss_pwron_failed =  false;
		priv->link_training_retval = 0;
		priv->hotplug_retval = 0;
#ifdef DP_DEBUG_ENABLE
		priv->need_lanes_force = false;
		priv->need_rate_force = false;
		priv->need_resolution_force = false;
#endif
		memset(&(priv->aux_rw), 0, sizeof(dp_aux_rw_t));

		hotplug->vs_force = DP_DSM_VS_PE_MASK;
		hotplug->pe_force = DP_DSM_VS_PE_MASK;

		priv->cur_hotplug_info = hotplug;
		priv->need_add_hotplug_to_list = true;

		priv->is_hotplug_running = true;
		priv->hotplug_state = 0;
		priv->hotplug_width = 0;
		priv->hotplug_high = 0;
	}

	if (priv->cur_hotplug_info != NULL) {
		do_gettimeofday(&(priv->cur_hotplug_info->time[index]));
#ifdef DP_DSM_DEBUG
		hwlog_info("%s: time[%d]=%ld,%ld\n", __func__, index,
			priv->cur_hotplug_info->time[index].tv_sec, priv->cur_hotplug_info->time[index].tv_usec);
#endif
	}

	// hotunplug, dp stop
	if (DP_PARAM_TIME_STOP == param) {
		dp_add_hotplug_info_to_list(priv, true);
	}
}

#ifdef DP_DEBUG_ENABLE
static void dp_reset_lanes_rate_force(dp_dsm_priv_t *priv, uint8_t *lanes_force, uint8_t *rate_force)
{
	if (NULL == priv) {
		hwlog_err("%s: priv is NULL!!!\n", __func__);
		return;
	}

	if (priv->need_lanes_force && (lanes_force != NULL)) {
		priv->need_lanes_force = false;
		*lanes_force = priv->lanes_force;
		hwlog_info("%s: set lanes_force %d force!!!\n", __func__, priv->lanes_force);
	}

	if (priv->need_rate_force && (rate_force != NULL)) {
		priv->need_rate_force = false;
		*rate_force = priv->rate_force;
		hwlog_info("%s: set rate_force %d force!!!\n", __func__, priv->rate_force);
	}
}
#endif

static void dp_set_param_basic_info(dp_dsm_priv_t *priv, dp_imonitor_param_t param, void *data)
{
	dp_hotplug_info_t *hotplug = NULL;

	DP_DSM_CHECK_HOTPLUG_PTR_P(priv, data);
	switch (param) {
	case DP_PARAM_WIDTH:
		hotplug->width = *(uint16_t *)data;
		priv->hotplug_width = hotplug->width;
		break;
	case DP_PARAM_HIGH:
		hotplug->high = *(uint16_t *)data;
		priv->hotplug_high = hotplug->high;
		break;
	case DP_PARAM_MAX_WIDTH:
		hotplug->max_width = *(uint16_t *)data;
		break;
	case DP_PARAM_MAX_HIGH:
		hotplug->max_high = *(uint16_t *)data;
		break;
	case DP_PARAM_PIXEL_CLOCK:
		hotplug->pixel_clock = *(int *)data;
		break;
	case DP_PARAM_FPS:
		hotplug->fps = *(uint8_t *)data;
		break;
	case DP_PARAM_MAX_RATE:
		hotplug->max_rate = *(uint8_t *)data;
		break;
	case DP_PARAM_MAX_LANES:
		hotplug->max_lanes = *(uint8_t *)data;
		break;
	case DP_PARAM_LINK_RATE:
#ifdef DP_DEBUG_ENABLE
		dp_reset_lanes_rate_force(priv, NULL, data);
#endif
		hotplug->rate = *(uint8_t *)data;
		break;
	case DP_PARAM_LINK_LANES:
#ifdef DP_DEBUG_ENABLE
		dp_reset_lanes_rate_force(priv, data, NULL);
#endif
		hotplug->lanes = *(uint8_t *)data;
		if (priv->is_hotplug_running) {
			dp_factory_set_link_rate_lanes(hotplug->rate, hotplug->lanes, hotplug->max_rate, hotplug->max_lanes);
		}
		break;
	case DP_PARAM_TU:
		dp_set_link_tu(hotplug, *(int *)data);
		break;
	case DP_PARAM_SOURCE_MODE:
		priv->cur_source_mode = *(bool *)data;
		hotplug->source_mode = priv->cur_source_mode;
		break;
	case DP_PARAM_USER_MODE:
		hotplug->user_mode = *(uint8_t *)data;
		break;
	case DP_PARAM_USER_FORMAT:
		hotplug->user_format = *(uint8_t *)data;
		break;
	case DP_PARAM_BASIC_AUDIO:
		hotplug->basic_audio = *(uint8_t *)data;
		break;
	case DP_PARAM_SAFE_MODE:
		hotplug->safe_mode = *(bool *)data;
		dp_set_hotplug_state(DP_LINK_STATE_SAFE_MODE);
		break;
	case DP_PARAM_READ_EDID_FAILED:
		hotplug->read_edid_retval = *(int *)data;
		dp_set_hotplug_state(DP_LINK_STATE_EDID_FAILED);
		break;
	case DP_PARAM_LINK_TRAINING_FAILED:
		priv->link_training_retval = *(int *)data;
		hotplug->link_training_retval = priv->link_training_retval;
		if (priv->is_hotplug_running) {
			dp_set_hotplug_state(DP_LINK_STATE_LINK_FAILED);
		} else {
			// 1. link retraining failed
			// 2. aux rw failed
			if (!dp_factory_mode_is_enable()) {
				dp_link_state_event(DP_LINK_STATE_LINK_FAILED);
			}
		}
		break;
	default:
		break;
	}
}

static void dp_set_param_extend_info(dp_dsm_priv_t *priv, dp_imonitor_param_t param, void *data)
{
	switch (param) {
	case DP_PARAM_EDID_BLOCK0:
	case DP_PARAM_EDID_BLOCK1:
	case DP_PARAM_EDID_BLOCK2:
	case DP_PARAM_EDID_BLOCK3:
		dp_set_edid_block(priv, param, data);
		break;
	case DP_PARAM_DPCD_RX_CAPS:
		dp_set_dpcd_rx_caps(priv, data);
		break;
	default:
		break;
	}
}

static void dp_set_param_hdcp(dp_dsm_priv_t *priv, dp_imonitor_param_t param, void *data)
{
	if (NULL == priv) {
		hwlog_err("%s: priv is NULL!!!\n", __func__);
		return;
	}

	switch (param) {
	case DP_PARAM_HDCP_VERSION:
		if (data != NULL) {
			priv->hdcp_version = *(uint8_t *)data;
		}
		break;
	case DP_PARAM_HDCP_KEY_S:
		do_gettimeofday(&(priv->hdcp_time));
		priv->hdcp_key = HDCP_KEY_SUCCESS;
		break;
	case DP_PARAM_HDCP_KEY_F:
		do_gettimeofday(&(priv->hdcp_time));
		priv->hdcp_key = HDCP_KEY_FAILED;
		if (!dp_factory_mode_is_enable()) {
			dp_link_state_event(DP_LINK_STATE_HDCP_FAILED);
		}

		// hdcp authentication failed
#ifndef DP_DEBUG_ENABLE
		DP_DSM_ADD_TO_IMONITOR_LIST(priv, rcf_hdcp_info, DP_IMONITOR_HDCP);
#else
		dp_add_info_to_imonitor_list(DP_IMONITOR_HDCP, priv);
#endif
		break;
	default:
		break;
	}
}

static void dp_set_hotplug(dp_dsm_priv_t *priv, int *retval)
{
	if ((NULL == priv) || (NULL == retval)) {
		hwlog_err("%s: priv or retval is NULL!!!\n", __func__);
		return;
	}

	priv->is_hotplug_running = false;
	priv->hotplug_retval = (*retval);
	if (priv->dss_pwron_failed) {
		priv->hotplug_retval = -DP_DSM_ERRNO_DSS_PWRON_FAILED;
	}

	if (priv->hotplug_retval < 0) {
		// if hotplug failed, not process hotunplug
		dp_add_hotplug_info_to_list(priv, false);

		if (priv->link_training_retval < 0) {
			// link training failed
#ifndef DP_DEBUG_ENABLE
			DP_DSM_ADD_TO_IMONITOR_LIST(priv, rcf_link_training_info, DP_IMONITOR_LINK_TRAINING);
#else
			dp_add_info_to_imonitor_list(DP_IMONITOR_LINK_TRAINING, priv);
#endif
		} else {
			// hotplug failed
#ifndef DP_DEBUG_ENABLE
			DP_DSM_ADD_TO_IMONITOR_LIST(priv, rcf_hotplug_info, DP_IMONITOR_HOTPLUG);
#else
			dp_add_info_to_imonitor_list(DP_IMONITOR_HOTPLUG, priv);
#endif
		}
	} else {
		// link training success
		dp_set_param_time_info(priv, DP_PARAM_TIME_LINK_SUCC);

		// hotplug success
		if (priv->cur_source_mode) { // same mode
			DP_DSM_ADD_TO_IMONITOR_LIST(priv, rcf_basic_info_of_same_mode, DP_IMONITOR_BASIC_INFO);
		} else { // diff mode
			DP_DSM_ADD_TO_IMONITOR_LIST(priv, rcf_basic_info_of_diff_mode, DP_IMONITOR_BASIC_INFO);
		}
	}

	// report link failed event
	dp_report_hotplug_state(priv);
}

static void dp_set_link_retraining(dp_dsm_priv_t *priv, int *retval)
{
	if ((NULL == priv) || (NULL == retval)) {
		hwlog_err("%s: priv or retval is NULL!!!\n", __func__);
		return;
	}

	priv->link_retraining_retval = (*retval);
	hwlog_info("%s: link retraining failed %d in dp_device_srs()!\n", __func__, priv->link_retraining_retval);

	// dp link failed in func dp_device_srs()
	if (priv->link_retraining_retval < 0) {
		if (!dp_factory_mode_is_enable()) {
			dp_link_state_event(DP_LINK_STATE_LINK_FAILED);
		}

#ifndef DP_DEBUG_ENABLE
		DP_DSM_ADD_TO_IMONITOR_LIST(priv, rcf_link_retraining_info, DP_IMONITOR_LINK_TRAINING);
#else
		dp_add_info_to_imonitor_list(DP_IMONITOR_LINK_TRAINING, priv);
#endif
	}
}

static void dp_set_param_diag(dp_dsm_priv_t *priv, dp_imonitor_param_t param, void *data)
{
	if (NULL == priv) {
		hwlog_err("%s: priv is NULL!!!\n", __func__);
		return;
	}

	switch (param) {
	case DP_PARAM_LINK_RETRAINING:
		priv->link_retraining_num++;
		priv->link_training_retval = 0;
		// maybe cause user's doubts by occasional interruptions
		//dp_link_state_event(DP_LINK_STATE_LINK_RETRAINING);
		break;
	case DP_PARAM_SAFE_MODE:
	case DP_PARAM_READ_EDID_FAILED:
	case DP_PARAM_LINK_TRAINING_FAILED:
		dp_set_param_basic_info(priv, param, data);
		break;
	case DP_PARAM_LINK_RETRAINING_FAILED:
		dp_set_link_retraining(priv, data);
		break;
	case DP_PARAM_HOTPLUG_RETVAL:
		dp_set_hotplug(priv, data);
		break;
	case DP_PARAM_IRQ_VECTOR:
		dp_set_irq_vector(priv, data);
		break;
	case DP_PARAM_SOURCE_SWITCH:
		dp_set_source_switch(priv, data);
		break;
	case DP_PARAM_DSS_PWRON_FAILED:
		priv->dss_pwron_failed = true;
		break;
	default:
		break;
	}
}

void dp_imonitor_set_param(dp_imonitor_param_t param, void *data)
{
	dp_dsm_priv_t *priv = g_dp_dsm_priv;

#ifdef DP_DSM_DEBUG
	hwlog_info("%s + param=%d\n", __func__, param);
#endif

	if (param <= DP_PARAM_TIME_MAX) {
		dp_set_param_time_info(priv, param);
	} else if ((param >= DP_PARAM_BASIC) && (param <= DP_PARAM_BASIC_MAX)) {
		dp_set_param_basic_info(priv, param, data);
	} else if ((param >= DP_PARAM_EXTEND) && (param <= DP_PARAM_EXTEND_MAX)) {
		dp_set_param_extend_info(priv, param, data);
	} else if ((param >= DP_PARAM_HDCP) && (param <= DP_PARAM_HDCP_MAX)) {
		dp_set_param_hdcp(priv, param, data);
	} else if ((param >= DP_PARAM_DIAG) && (param <= DP_PARAM_DIAG_MAX)) {
		dp_set_param_diag(priv, param, data);
	} else { // other params
		hwlog_info("%s: unkown param(%d), skip!!!\n", __func__, param);
	}

#ifdef DP_DSM_DEBUG
	hwlog_info("%s -\n", __func__);
#endif
}
EXPORT_SYMBOL_GPL(dp_imonitor_set_param);

void dp_imonitor_set_param_aux_rw(bool rw, bool i2c, uint32_t addr, uint32_t len, int retval)
{
	dp_dsm_priv_t *priv = g_dp_dsm_priv;

	if (NULL == priv) {
		hwlog_err("%s: priv is NULL!!!\n", __func__);
		return;
	}

	do_gettimeofday(&(priv->aux_rw.time));

	priv->aux_rw.rw     = rw;
	priv->aux_rw.i2c    = i2c;
	priv->aux_rw.addr   = addr;
	priv->aux_rw.len    = len;
	priv->aux_rw.retval = retval;

	if (priv->is_hotplug_running) {
		dp_set_hotplug_state(DP_LINK_STATE_AUX_FAILED);
	}
}
EXPORT_SYMBOL_GPL(dp_imonitor_set_param_aux_rw);

void dp_imonitor_set_param_timing(uint16_t h_active, uint16_t v_active, uint32_t pixel_clock, uint8_t vesa_id)
{
	dp_dsm_priv_t *priv = g_dp_dsm_priv;
	dp_timing_info_t *timing = NULL;

	if (NULL == priv) {
		hwlog_err("%s: priv is NULL!!!\n", __func__);
		return;
	}

	timing = (dp_timing_info_t *)kzalloc(sizeof(dp_timing_info_t), GFP_KERNEL);
	if (NULL == timing) {
		hwlog_err("%s: kzalloc timing failed!!!\n", __func__);
		return;
	}

	timing->h_active    = h_active;
	timing->v_active    = v_active;
	timing->pixel_clock = pixel_clock;
	timing->vesa_id     = vesa_id;
	list_add_tail(&(timing->list), &(priv->timing_list_head));
}
EXPORT_SYMBOL_GPL(dp_imonitor_set_param_timing);

void dp_imonitor_set_param_err_count(uint16_t lane0_err, uint16_t lane1_err, uint16_t lane2_err, uint16_t lane3_err)
{
	dp_dsm_priv_t *priv = g_dp_dsm_priv;

	if (NULL == priv) {
		hwlog_err("%s: priv is NULL!!!\n", __func__);
		return;
	}

	// 15-bit value storing the symbol error count of Lane 0~3
	priv->lane0_err += lane0_err;
	priv->lane1_err += lane1_err;
	priv->lane2_err += lane2_err;
	priv->lane3_err += lane3_err;
	hwlog_info("%s: err_count(0x%x,0x%x,0x%x,0x%x) happened!!!\n", __func__,
		priv->lane0_err, priv->lane1_err, priv->lane2_err, priv->lane3_err);
}
EXPORT_SYMBOL_GPL(dp_imonitor_set_param_err_count);

// NOTE:
// 1. record vs and pe for imonitor
// 2. reset vs and pe force when to debug(eng build)
void dp_imonitor_set_param_vs_pe(int index, uint8_t *vs, uint8_t *pe)
{
	dp_dsm_priv_t *priv = g_dp_dsm_priv;
	dp_hotplug_info_t *hotplug = NULL;
#ifdef DP_DEBUG_ENABLE
	uint8_t vs_force = 0;
	uint8_t pe_force = 0;
#endif

	if ((NULL == priv) || (NULL == priv->cur_hotplug_info) || (NULL == vs) || (NULL == pe)) {
		hwlog_err("%s: priv, hotplug, vs or pe is NULL!!!\n", __func__);
		return;
	}
	hotplug = priv->cur_hotplug_info;

	if ((index < 0) || (index >= (int)DP_DSM_VS_PE_NUM)) {
		hwlog_err("%s: invalid index %d!!!\n", __func__, index);
		return;
	}

	hotplug->vp.vs_pe[index].vswing = *vs;
	hotplug->vp.vs_pe[index].preemp = *pe;

	// for debug: set vs or pe force
#ifdef DP_DEBUG_ENABLE
	if (0 == dp_debug_get_vs_pe_force(&vs_force, &pe_force)) {
		hotplug->vs_force = vs_force;
		hotplug->pe_force = pe_force;

		hotplug->vp_force.vs_pe[index].vswing = vs_force;
		hotplug->vp_force.vs_pe[index].preemp = pe_force;

		*vs = vs_force;
		*pe = pe_force;
		if (0 == index) {
			hwlog_info("%s: set vs %d or pe %d force!!!\n", __func__, vs_force, pe_force);
		}
	}
#endif // DP_DEBUG_ENABLE
}
EXPORT_SYMBOL_GPL(dp_imonitor_set_param_vs_pe);

// for debug: reset resolution force
void dp_imonitor_set_param_resolution(uint8_t *user_mode, uint8_t *user_format)
{
#ifndef DP_DEBUG_ENABLE
	UNUSED(user_mode);
	UNUSED(user_format);
#else
	dp_dsm_priv_t *priv = g_dp_dsm_priv;

	if (NULL == priv) {
		hwlog_err("%s: priv is NULL!!!\n", __func__);
		return;
	}

	if (priv->need_resolution_force) {
		*user_mode   = priv->user_mode;
		*user_format = priv->user_format;
		hwlog_info("%s: set user_mode %d and user_format %d force!!!\n", __func__, priv->user_mode, priv->user_format);
	}
#endif // DP_DEBUG_ENABLE
}
EXPORT_SYMBOL_GPL(dp_imonitor_set_param_resolution);

#ifdef DP_DEBUG_ENABLE
static bool dp_check_typec_cable_in_out_existed(dp_dsm_priv_t *priv)
{
	if (NULL == priv) {
		hwlog_err("%s: priv is NULL!!!\n", __func__);
		return false;
	}

	if (0 == priv->tpyec_in_time.tv_sec) {
		return false;
	}

	return true;
}

int dp_get_pd_event_result(char *buffer, int size)
{
	dp_dsm_priv_t *priv = g_dp_dsm_priv;
	struct list_head *pos = NULL;
	dp_connected_event_t *event = NULL;
	char datetime[DP_DSM_DATETIME_SIZE] = {0};
	int earliest_point = 0;
	int count = 0;
	int ret = 0;
	int i = 0;

	if (NULL == priv) {
		hwlog_err("%s: priv is NULL!!!\n", __func__);
		return -EINVAL;
	}

	if (g_typec_in_out.in_out_num > 0) {
		earliest_point = DP_DSM_EARLIEST_TIME_POINT(g_typec_in_out.in_out_num, DP_DSM_TYPEC_IN_OUT_NUM);
		count = MIN(g_typec_in_out.in_out_num, DP_DSM_TYPEC_IN_OUT_NUM);

		ret += dp_debug_append_info(buffer, size, "[**] %-19s %-19s\n", "typec_in_time", "typec_out_time");
		for (i = 0; i < count; i++) {
			dp_timeval_to_datetime_str(g_typec_in_out.in_time[i], "tpyec_in", datetime, DP_DSM_DATETIME_SIZE);
			ret += dp_debug_append_info(buffer, size, "[%02d] %-19s", i, datetime);

			dp_timeval_to_datetime_str(g_typec_in_out.out_time[i], "tpyec_out", datetime, DP_DSM_DATETIME_SIZE);
			if (i != earliest_point) {
				ret += dp_debug_append_info(buffer, size, " %-19s\n", datetime);
			} else {
				ret += dp_debug_append_info(buffer, size, " %-19s*\n", datetime);
			}
		}
		ret += dp_debug_append_info(buffer, size, "\n");
	}

	if (!dp_check_typec_cable_in_out_existed(priv)) {
		ret += dp_debug_append_info(buffer, size, "pd_event not found!\n");
		goto err_out;
	}

	dp_timeval_to_datetime_str(priv->tpyec_in_time, "tpyec_in", datetime, DP_DSM_DATETIME_SIZE);
	ret += dp_debug_append_info(buffer, size, "tpyec_in_time: %s\n", datetime);
	dp_timeval_to_datetime_str(priv->tpyec_out_time, "tpyec_out", datetime, DP_DSM_DATETIME_SIZE);
	ret += dp_debug_append_info(buffer, size, "tpyec_out_time: %s\n", datetime);

	ret += dp_debug_append_info(buffer, size, "irq_hpd_num: %d\n", priv->irq_hpd_num);
	ret += dp_debug_append_info(buffer, size, "irq_short_num: %d\n", priv->irq_short_num);
	ret += dp_debug_append_info(buffer, size, "dp_in_num: %d\n", priv->dp_in_num);
	ret += dp_debug_append_info(buffer, size, "dp_out_num: %d\n", priv->dp_out_num);
	ret += dp_debug_append_info(buffer, size, "link_retraining_num: %d\n", priv->link_retraining_num);
	ret += dp_debug_append_info(buffer, size, "\n");

	i = 0;
	ret += dp_debug_append_info(buffer, size, "[**] %-19s %-12s %-12s %-12s %-12s %-12s\n",
		"time", "irq_type", "cur_mode", "mode_type", "dev_type", "typec_orien");
	list_for_each(pos, &(priv->event_list_head)) {
		event = list_entry(pos, dp_connected_event_t, list);

		dp_timeval_to_datetime_str(event->time, "pd_event", datetime, DP_DSM_DATETIME_SIZE);
		ret += dp_debug_append_info(buffer, size, "[%02d] %-19s %-12u %-12u %-12u %-12u %-12u\n",
			i, datetime, event->irq_type, event->cur_mode, event->mode_type, event->dev_type, event->typec_orien);
		i++;
	}

	// irq vector
	if (priv->irq_vector_num > 0) {
		earliest_point = DP_DSM_EARLIEST_TIME_POINT(priv->irq_vector_num, DP_DSM_IRQ_VECTOR_NUM);
		count = MIN(priv->irq_vector_num, DP_DSM_IRQ_VECTOR_NUM);

		ret += dp_debug_append_info(buffer, size, "irq vector(%d):", priv->irq_vector_num);
		for (i = 0; i < count; i++) {
			if (i != earliest_point) {
				ret += dp_debug_append_info(buffer, size, " %u", priv->irq_vector[i]);
			} else {
				ret += dp_debug_append_info(buffer, size, " %u*", priv->irq_vector[i]);
			}
		}
		ret += dp_debug_append_info(buffer, size, "\n");
	}

	// hdcp authentication
	if (priv->hdcp_key != HDCP_KEY_INVALID) {
		dp_timeval_to_datetime_str(priv->hdcp_time, "hdcp_auth", datetime, DP_DSM_DATETIME_SIZE);
		ret += dp_debug_append_info(buffer, size, "hdcp_auth_time: %s\n", datetime);

		ret += dp_debug_append_info(buffer, size, "hdcp_version: %s\n",
			(3 == priv->hdcp_version) ? "1.3" : "2.2 or other");
		ret += dp_debug_append_info(buffer, size, "hdcp_key: %s\n",
			(HDCP_KEY_FAILED == priv->hdcp_key) ? "failed" : "success");
	}

err_out:
	if (ret < 0) {
		return -EFAULT;
	}

	return strlen(buffer);
}
EXPORT_SYMBOL_GPL(dp_get_pd_event_result);

int dp_get_hotplug_result(char *buffer, int size)
{
	dp_dsm_priv_t *priv = g_dp_dsm_priv;
	struct list_head *pos = NULL;
	dp_hotplug_info_t *hotplug = NULL;
	char datetime[DP_DSM_DATETIME_SIZE] = {0};
	int earliest_point = 0;
	int count = 0;
	int ret = 0;
	int i = 0;

	if (NULL == priv) {
		hwlog_err("%s: priv is NULL!!!\n", __func__);
		return -EINVAL;
	}

	if (!dp_check_typec_cable_in_out_existed(priv)) {
		ret += dp_debug_append_info(buffer, size, "hotplug not found!\n");
		goto err_out;
	}

	ret += dp_debug_append_info(buffer, size, "cable max rate: 2\n");
	if (TCPC_DP == priv->tpyec_cable_type) {
		ret += dp_debug_append_info(buffer, size, "cable max lanes: 4\n");
	} else { // TCPC_USB31_AND_DP_2LINE
		ret += dp_debug_append_info(buffer, size, "cable max lanes: 2\n");
	}

	if (priv->cur_hotplug_info != NULL) {
		hotplug = priv->cur_hotplug_info;

		if (hotplug->max_rate > 0) {
			ret += dp_debug_append_info(buffer, size, "sink max rate: %u\n", hotplug->max_rate);
			ret += dp_debug_append_info(buffer, size, "sink max lanes: %u\n", hotplug->max_lanes);
		}

		if (hotplug->max_width > 0) {
			ret += dp_debug_append_info(buffer, size, "mid: %s\n", hotplug->mid);
			ret += dp_debug_append_info(buffer, size, "monitor: %s\n", hotplug->monitor);
			ret += dp_debug_append_info(buffer, size, "max_width: %u\n", hotplug->max_width);
			ret += dp_debug_append_info(buffer, size, "max_high: %u\n", hotplug->max_high);
			ret += dp_debug_append_info(buffer, size, "pixel_clock: %d\n", hotplug->pixel_clock);
		}
	}
	ret += dp_debug_append_info(buffer, size, "\n");

	ret += dp_debug_append_info(buffer, size, "[**] %-8s %-10s %-14s %-9s %-6s %-5s %-5s %-5s %-4s %-5s %-5s %-9s %-10s %-6s\n",
		"status",
		"start_time", "link_succ_time", "stop_time",
		"source", "width", "high", "fps", "rate", "lanes", "audio",
		"safe_mode", "edid_state", "reduce");

	list_for_each(pos, &(priv->hotplug_list_head)) {
		hotplug = list_entry(pos, dp_hotplug_info_t, list);

		dp_timeval_to_time_str(hotplug->time[DP_PARAM_TIME_START - DP_PARAM_TIME],
			"start_time", datetime, DP_DSM_DATETIME_SIZE);
		if (priv->hotplug_retval < 0) {
			if (priv->aux_rw.retval < 0) {
				ret += dp_debug_append_info(buffer, size, "[%02d] %-8s %-10s(aux rw:%d, i2c:%d, addr:0x%x, len:%u, retval:%d)\n",
					i, "failed", datetime,
					priv->aux_rw.rw, priv->aux_rw.i2c, priv->aux_rw.addr, priv->aux_rw.len, priv->aux_rw.retval);
			} else if (priv->link_training_retval < 0) {
				ret += dp_debug_append_info(buffer, size, "[%02d] %-8s %-10s(link training failed %d)\n",
					i, "failed", datetime, priv->link_training_retval);
			} else if (priv->dss_pwron_failed) {
				ret += dp_debug_append_info(buffer, size, "[%02d] %-8s %-10s(dss pwron failed)\n",
					i, "failed", datetime);
			} else {
				ret += dp_debug_append_info(buffer, size, "[%02d] %-8s %-10s(hotplug failed %d)\n",
					i, "failed", datetime, priv->hotplug_retval);
			}
		} else {
			if (hotplug->hotplug_retval < 0) {
				ret += dp_debug_append_info(buffer, size, "[%02d] %-8s %-10s(hotplug failed %d)\n",
					i,  "failed", datetime, hotplug->hotplug_retval);
			} else {
				ret += dp_debug_append_info(buffer, size, "[%02d] %-8s %-10s", i, "success", datetime);

				dp_timeval_to_time_str(hotplug->time[DP_PARAM_TIME_LINK_SUCC - DP_PARAM_TIME],
					"link_succ_time", datetime, DP_DSM_DATETIME_SIZE);
				ret += dp_debug_append_info(buffer, size, " %-14s", datetime);

				dp_timeval_to_time_str(hotplug->time[DP_PARAM_TIME_STOP - DP_PARAM_TIME],
					"stop_time", datetime, DP_DSM_DATETIME_SIZE);
				ret += dp_debug_append_info(buffer, size, " %-9s", datetime);

				ret += dp_debug_append_info(buffer, size, " %-6s %-5u %-5u %-5u %-4u %-5u %-5u %-9s %-10s %-6s\n",
					hotplug->source_mode ? "same" : "diff",
					hotplug->width, hotplug->high, hotplug->fps, hotplug->rate, hotplug->lanes, hotplug->basic_audio,
					hotplug->safe_mode ? "safe" : "normal",
					(0 == hotplug->read_edid_retval) ? "success" : "failed",
					(hotplug->tu_fail >= DP_DSM_TU_FAIL) ? "reduce" : "normal");
			}
		}

		i++;
	}

	// source switch
	if (priv->source_switch_num > 0) {
		earliest_point = DP_DSM_EARLIEST_TIME_POINT(priv->source_switch_num, DP_DSM_SOURCE_SWITCH_NUM);
		count = MIN(priv->source_switch_num, DP_DSM_SOURCE_SWITCH_NUM);

		ret += dp_debug_append_info(buffer, size, "source switch(%d):", priv->source_switch_num);
		for (i = 0; i < count; i++) {
			if (i != earliest_point) {
				ret += dp_debug_append_info(buffer, size, " %u", priv->source_switch[i]);
			} else {
				ret += dp_debug_append_info(buffer, size, " %u*", priv->source_switch[i]);
			}
		}
		ret += dp_debug_append_info(buffer, size, "\n");
	}

	// dp link failed in func dp_device_srs()
	if (priv->link_retraining_retval < 0) {
		ret += dp_debug_append_info(buffer, size, "dp resume failed by press powerkey.\n");
	}

	// hpd not existed
	if (!g_imonitor_report.hpd_existed) {
		ret += dp_debug_append_info(buffer, size, "hpd not existed.\n");
	}

err_out:
	if (ret < 0) {
		return -EFAULT;
	}

	return strlen(buffer);
}
EXPORT_SYMBOL_GPL(dp_get_hotplug_result);

int dp_get_timing_info_result(char *buffer, int size)
{
	dp_dsm_priv_t *priv = g_dp_dsm_priv;
	struct list_head *pos = NULL;
	dp_timing_info_t *timing = NULL;
	int ret = 0;
	int i = 0;

	if (NULL == priv) {
		hwlog_err("%s: priv is NULL!!!\n", __func__);
		return -EINVAL;
	}

	// timing info
	ret += dp_debug_append_info(buffer, size, "[**] %-8s %-8s %-11s %-7s\n",
		"h_active", "v_active", "pixel_clock", "vesa_id");

	list_for_each(pos, &(priv->timing_list_head)) {
		timing = list_entry(pos, dp_timing_info_t, list);

		ret += dp_debug_append_info(buffer, size, "[%02d] %-8u %-8u %-11u %-7u\n",
			i, timing->h_active, timing->v_active, timing->pixel_clock, timing->vesa_id);
		i++;
	}

	if (ret < 0) {
		return -EFAULT;
	}

	return strlen(buffer);
}
EXPORT_SYMBOL_GPL(dp_get_timing_info_result);

static bool dp_is_vs_pe_force(dp_hotplug_info_t *hotplug)
{
	if ((hotplug != NULL) && (hotplug->vs_force != DP_DSM_VS_PE_MASK) && (hotplug->pe_force != DP_DSM_VS_PE_MASK)) {
		return true;
	}

	return false;
}

int dp_get_vs_pe_result(char *buffer, int size)
{
	dp_dsm_priv_t *priv = g_dp_dsm_priv;
	dp_hotplug_info_t *hotplug = NULL;
	int ret = 0;
	int i = 0;

	if (NULL == priv) {
		hwlog_err("%s: priv is NULL!!!\n", __func__);
		return -EINVAL;
	}

	if (NULL == priv->cur_hotplug_info) {
		ret += dp_debug_append_info(buffer, size, "hotplug not found!\n");
		goto err_out;
	}
	hotplug = priv->cur_hotplug_info;

	if (priv->hotplug_retval < 0) {
		ret += dp_debug_append_info(buffer, size, "hotplug failed!\n");
		goto err_out;
	}

	if (!dp_is_vs_pe_force(hotplug)) {
		ret += dp_debug_append_info(buffer, size, "vs_force: not set!\n");
		ret += dp_debug_append_info(buffer, size, "pe_force: not set!\n");
	} else {
		ret += dp_debug_append_info(buffer, size, "vs_force: %u\n", hotplug->vs_force);
		ret += dp_debug_append_info(buffer, size, "pe_force: %u\n", hotplug->pe_force);
	}

	// vs and pe
	ret += dp_debug_append_info(buffer, size, "[*] vs pe\n");
	for (i = 0; i < hotplug->lanes; i++) {
		ret += dp_debug_append_info(buffer, size, "[%d] %-2u %-2u\n",
			i, hotplug->vp.vs_pe[i].vswing, hotplug->vp.vs_pe[i].preemp);
	}

	// vs_force and pe_force
	if (dp_is_vs_pe_force(hotplug)) {
		ret += dp_debug_append_info(buffer, size, "[*] vs_force pe_force\n");
		for (i = 0; i < hotplug->lanes; i++) {
			ret += dp_debug_append_info(buffer, size, "[%d] %-8u %-8u\n",
				i, hotplug->vp_force.vs_pe[i].vswing, hotplug->vp_force.vs_pe[i].preemp);
		}
	}

err_out:
	if (ret < 0) {
		return -EFAULT;
	}

	return strlen(buffer);
}
EXPORT_SYMBOL_GPL(dp_get_vs_pe_result);
#endif // DP_DEBUG_ENABLE

static int dp_imonitor_prepare_basic_info(struct imonitor_eventobj *obj, void *data)
{
	dp_dsm_priv_t *priv = DP_DSM_IMONITOR_PREPARE_PRIV;
	dp_hotplug_info_t *hotplug = NULL;
	int ret = 0;

	DP_DSM_CHECK_HOTPLUG_PTR_R(priv, obj);
#ifdef DP_DSM_DEBUG
	// dock or dongle
	hwlog_info("%s: tpyec_cable_type=%u\n", __func__, priv->tpyec_cable_type);

	hwlog_info("%s: mid=%s\n",         __func__, hotplug->mid);
	hwlog_info("%s: monitor=%s\n",     __func__, hotplug->monitor);
	hwlog_info("%s: width=%u\n",       __func__, hotplug->width);
	hwlog_info("%s: high=%u\n",        __func__, hotplug->high);
	hwlog_info("%s: max_width=%u\n",   __func__, hotplug->max_width);
	hwlog_info("%s: max_high=%u\n",    __func__, hotplug->max_high);
	hwlog_info("%s: pixel_clock=%d\n", __func__, hotplug->pixel_clock);
	hwlog_info("%s: fps=%u\n",         __func__, hotplug->fps);

    // sink max lanes/rate
	hwlog_info("%s: max_rate=%u\n",    __func__, hotplug->max_rate);
	hwlog_info("%s: max_lanes=%u\n",   __func__, hotplug->max_lanes);
	// actual link lanes/rate
	hwlog_info("%s: rate=%u\n",        __func__, hotplug->rate);
	hwlog_info("%s: lanes=%u\n",       __func__, hotplug->lanes);
	hwlog_info("%s: tu=%d\n",          __func__, hotplug->tu);
	hwlog_info("%s: tu_fail=%d\n",     __func__, hotplug->tu_fail);
	hwlog_info("%s: vs_pe=0x%08x\n",   __func__, hotplug->vp.vswing_preemp);

	hwlog_info("%s: source_mode=%d\n", __func__, hotplug->source_mode);
	hwlog_info("%s: user_mode=%u\n",   __func__, hotplug->user_mode);
	hwlog_info("%s: user_format=%u\n", __func__, hotplug->user_format);
	hwlog_info("%s: basic_audio=%u\n", __func__, hotplug->basic_audio);

	hwlog_info("%s: edid_version=0x%x\n",  __func__, hotplug->edid_version);
	hwlog_info("%s: dpcd_revision=0x%x\n", __func__, hotplug->dpcd_revision);

	hwlog_info("%s: safe_mode=%d\n",        __func__, hotplug->safe_mode);
	hwlog_info("%s: read_edid_retval=%d\n", __func__, hotplug->read_edid_retval);

	hwlog_info("%s: current_edid_num=%d\n",       __func__, hotplug->current_edid_num);
	hwlog_info("%s: edid_header_num=0x%08x\n",    __func__, hotplug->edid_check.u32.header_num);
	hwlog_info("%s: edid_checksum_num=0x%08x\n",  __func__, hotplug->edid_check.u32.checksum_num);
	hwlog_info("%s: edid_header_flag=0x%08x\n",   __func__, hotplug->edid_check.u32.header_flag);
	hwlog_info("%s: edid_checksum_flag=0x%08x\n", __func__, hotplug->edid_check.u32.checksum_flag);
	hwlog_info("%s: edid_ext_blocks_num=%d\n",    __func__, hotplug->edid_check.u32.ext_blocks_num);
	hwlog_info("%s: edid_ext_blocks=0x%02x\n",    __func__, hotplug->edid_check.u32.ext_blocks);
#endif

	ret += imonitor_set_param_integer_v2(obj, "CableType", (long)priv->tpyec_cable_type);

	ret += imonitor_set_param_string_v2(obj,  "Mid",      hotplug->mid);
	ret += imonitor_set_param_string_v2(obj,  "Monitor",  hotplug->monitor);
	ret += imonitor_set_param_integer_v2(obj, "Width",    (long)hotplug->width);
	ret += imonitor_set_param_integer_v2(obj, "High",     (long)hotplug->high);
	ret += imonitor_set_param_integer_v2(obj, "MaxWidth", (long)hotplug->max_width);
	ret += imonitor_set_param_integer_v2(obj, "MaxHigh",  (long)hotplug->max_high);
	ret += imonitor_set_param_integer_v2(obj, "PixelClk", (long)hotplug->pixel_clock);
	ret += imonitor_set_param_integer_v2(obj, "Fps",      (long)hotplug->fps);

	ret += imonitor_set_param_integer_v2(obj, "MaxRate",   (long)hotplug->max_rate);
	ret += imonitor_set_param_integer_v2(obj, "MaxLanes",  (long)hotplug->max_lanes);
	ret += imonitor_set_param_integer_v2(obj, "LinkRate",  (long)hotplug->rate);
	ret += imonitor_set_param_integer_v2(obj, "LinkLanes", (long)hotplug->lanes);
	ret += imonitor_set_param_integer_v2(obj, "Tu",        (long)hotplug->tu);
	ret += imonitor_set_param_integer_v2(obj, "TuFail",    (long)hotplug->tu_fail);
	ret += imonitor_set_param_integer_v2(obj, "VsPe",      (long)hotplug->vp.vswing_preemp);

	ret += imonitor_set_param_integer_v2(obj, "SrcMode",  (long)hotplug->source_mode);
	ret += imonitor_set_param_integer_v2(obj, "UserMode", (long)hotplug->user_mode);
	ret += imonitor_set_param_integer_v2(obj, "UserFmt",  (long)hotplug->user_format);
	ret += imonitor_set_param_integer_v2(obj, "Audio",    (long)hotplug->basic_audio);

	ret += imonitor_set_param_integer_v2(obj, "EdidVer", (long)hotplug->edid_version);
	ret += imonitor_set_param_integer_v2(obj, "DpcdVer", (long)hotplug->dpcd_revision);

	ret += imonitor_set_param_integer_v2(obj, "SafeMode", (long)hotplug->safe_mode);
	ret += imonitor_set_param_integer_v2(obj, "RedidRet", (long)hotplug->read_edid_retval);

	ret += imonitor_set_param_integer_v2(obj, "EdidNum",   (long)hotplug->current_edid_num);
	ret += imonitor_set_param_integer_v2(obj, "HeadNum",   (long)hotplug->edid_check.u32.header_num);
	ret += imonitor_set_param_integer_v2(obj, "ChksNum",   (long)hotplug->edid_check.u32.checksum_num);
	ret += imonitor_set_param_integer_v2(obj, "HeadFlag",  (long)hotplug->edid_check.u32.header_flag);
	ret += imonitor_set_param_integer_v2(obj, "ChksFlag",  (long)hotplug->edid_check.u32.checksum_flag);
	ret += imonitor_set_param_integer_v2(obj, "ExtBlkNum", (long)hotplug->edid_check.u32.ext_blocks_num);
	ret += imonitor_set_param_integer_v2(obj, "ExtBlks",   (long)hotplug->edid_check.u32.ext_blocks);

	return ret;
}

static int dp_imonitor_prepare_time_info(struct imonitor_eventobj *obj, void *data)
{
	dp_dsm_priv_t *priv = DP_DSM_IMONITOR_PREPARE_PRIV;
	char in_time[DP_DSM_DATETIME_SIZE]   = {0};
	char out_time[DP_DSM_DATETIME_SIZE]  = {0};
	char link_time[DP_DSM_DATETIME_SIZE] = {0};
	char same_time[DP_DSM_DATETIME_SIZE] = {0};
	char diff_time[DP_DSM_DATETIME_SIZE] = {0};
	char source_switch[DP_DSM_SOURCE_SWITCH_NUM + 1] = {0};
	int count = 0;
	int ret = 0;

	if ((NULL == priv) || (NULL == obj)) {
		hwlog_err("%s: priv or obj is NULL!!!\n", __func__);
		return -EINVAL;
	}

	// typec in & out time
	dp_timeval_to_datetime_str(priv->tpyec_in_time, "tpyec_in", in_time, DP_DSM_DATETIME_SIZE);
	dp_timeval_to_datetime_str(priv->tpyec_out_time, "tpyec_out", out_time, DP_DSM_DATETIME_SIZE);
	DP_DSM_TIMEVAL_WITH_TZ(priv->same_mode_time);
	DP_DSM_TIMEVAL_WITH_TZ(priv->diff_mode_time);
	dp_timeval_to_datetime_str(priv->same_mode_time, "same_mode_time", same_time, DP_DSM_DATETIME_SIZE);
	dp_timeval_to_datetime_str(priv->diff_mode_time, "diff_mode_time", diff_time, DP_DSM_DATETIME_SIZE);
	priv->link_max_time.tv_sec = DP_DSM_TIME_MS_PERCENT(priv->link_max_time.tv_usec);
	DP_DSM_TIMEVAL_WITH_TZ(priv->link_max_time);
	dp_timeval_to_datetime_str(priv->link_max_time,  "link_max_time",  link_time, DP_DSM_DATETIME_SIZE);

	count = MIN(priv->source_switch_num, DP_DSM_SOURCE_SWITCH_NUM);
	dp_data_bin_to_str(source_switch, DP_DSM_SOURCE_SWITCH_NUM + 1,
		priv->source_switch, count, DP_DSM_BIN_TO_STR_SHIFT8);

#ifdef DP_DSM_DEBUG
	hwlog_info("%s: tpyec_in_time=%s\n",  __func__, in_time);
	hwlog_info("%s: tpyec_out_time=%s\n", __func__, out_time);
	hwlog_info("%s: link_time=%s\n",      __func__, link_time);
	hwlog_info("%s: same_mode_time=%s\n", __func__, same_time);
	hwlog_info("%s: diff_mode_time=%s\n", __func__, diff_time);

	hwlog_info("%s: source_switch_num=%d\n", __func__, priv->source_switch_num);
	hwlog_info("%s: source_switch=%s\n",     __func__, source_switch);
#endif

	ret += imonitor_set_param_string_v2(obj, "InTime",   in_time);
	ret += imonitor_set_param_string_v2(obj, "OutTime",  out_time);
	ret += imonitor_set_param_string_v2(obj, "LinkTime", link_time);
	ret += imonitor_set_param_string_v2(obj, "SameTime", same_time);
	ret += imonitor_set_param_string_v2(obj, "DiffTime", diff_time);

	ret += imonitor_set_param_integer_v2(obj, "SwNum", (long)priv->source_switch_num);
	ret += imonitor_set_param_string_v2(obj,  "SrcSw", source_switch);

	// the key information of the last day
	if (g_imonitor_report.report_skip_existed) {
		int i = 0;

		for (i = 0; i < DP_IMONITOR_TYPE_NUM; i++) {
			hwlog_info("%s: last_report_num[%d]=%u\n", __func__, i, g_imonitor_report.last_report_num[i]);
		}
		hwlog_info("%s: last_link_min_time=%ld\n",    __func__, g_imonitor_report.last_link_min_time.tv_usec);
		hwlog_info("%s: last_link_max_time=%ld\n",    __func__, g_imonitor_report.last_link_max_time.tv_usec);
		hwlog_info("%s: last_same_mode_time=%ld\n",   __func__, g_imonitor_report.last_same_mode_time.tv_sec);
		hwlog_info("%s: last_diff_mode_time=%ld\n",   __func__, g_imonitor_report.last_diff_mode_time.tv_sec);
		hwlog_info("%s: last_source_switch_num=%d\n", __func__, g_imonitor_report.last_source_switch_num);

		g_imonitor_report.report_skip_existed = false;
		memset(g_imonitor_report.last_report_num, 0, sizeof(uint32_t) * DP_IMONITOR_TYPE_NUM);
	}

	return ret;
}

static int dp_imonitor_prepare_extend_info(struct imonitor_eventobj *obj, void *data)
{
	dp_dsm_priv_t *priv = DP_DSM_IMONITOR_PREPARE_PRIV;
	dp_hotplug_info_t *hotplug = NULL;
	char edid[DP_DSM_EDID_BLOCK_SIZE * DP_DSM_BIN_TO_STR_TIMES + 1] = {0};
	char param[DP_DSM_PARAM_NAME_MAX] = {0};
	int ret = 0;
	int i = 0;

	DP_DSM_CHECK_HOTPLUG_PTR_R(priv, obj);
#ifdef DP_DSM_DEBUG
	hwlog_info("%s: current_edid_num=%d\n",       __func__, hotplug->current_edid_num);
	hwlog_info("%s: edid_header_flag=0x%08x\n",   __func__, hotplug->edid_check.u32.header_flag);
	hwlog_info("%s: edid_checksum_flag=0x%08x\n", __func__, hotplug->edid_check.u32.checksum_flag);
#endif

	ret += imonitor_set_param_integer_v2(obj, "EdidNum",  (long)hotplug->current_edid_num);
	ret += imonitor_set_param_integer_v2(obj, "HeadFlag", (long)hotplug->edid_check.u32.header_flag);
	ret += imonitor_set_param_integer_v2(obj, "ChksFlag", (long)hotplug->edid_check.u32.checksum_flag);

	// dp edid block info
	for (i = 0; i < hotplug->current_edid_num; i++) {
		dp_data_bin_to_str(edid, DP_DSM_EDID_BLOCK_SIZE * DP_DSM_BIN_TO_STR_TIMES + 1,
			hotplug->edid[i], DP_DSM_EDID_BLOCK_SIZE, DP_DSM_BIN_TO_STR_SHIFT4);

		snprintf(param, (unsigned long)DP_DSM_PARAM_NAME_MAX, "EdidBlk%d", i);
		ret += imonitor_set_param_string_v2(obj, param, edid);
#ifdef DP_DSM_DEBUG
		hwlog_info("%s: edid[%d]=%s\n", __func__, i, edid);
#endif
	}

	return ret;
}

static int dp_imonitor_prepare_hpd_info(struct imonitor_eventobj *obj, void *data)
{
	dp_dsm_priv_t *priv = DP_DSM_IMONITOR_PREPARE_PRIV;
	char in_time[DP_DSM_DATETIME_SIZE] = {0};
	char out_time[DP_DSM_DATETIME_SIZE] = {0};
	char dev_type[DP_DSM_TCA_DEV_TYPE_NUM + 1] = {0};
	char irq_vector[DP_DSM_IRQ_VECTOR_NUM + 1] = {0};
	char source_switch[DP_DSM_SOURCE_SWITCH_NUM + 1] = {0};
	int count = 0;
	int ret = 0;

	if ((NULL == priv) || (NULL == obj)) {
		hwlog_err("%s: priv or obj is NULL!!!\n", __func__);
		return -EINVAL;
	}

	dp_timeval_to_datetime_str(priv->tpyec_in_time, "tpyec_in", in_time, DP_DSM_DATETIME_SIZE);
	dp_timeval_to_datetime_str(priv->tpyec_out_time, "tpyec_out", out_time, DP_DSM_DATETIME_SIZE);
#ifdef DP_DSM_DEBUG
	hwlog_info("%s: tpyec_in_time=%s\n",  __func__, in_time);
	hwlog_info("%s: tpyec_out_time=%s\n", __func__, out_time);

	hwlog_info("%s: event_num=%d\n",           __func__, priv->event_num);
	hwlog_info("%s: tpyec_cable_type=%u\n",    __func__, priv->tpyec_cable_type);
	hwlog_info("%s: irq_hpd_num=%d\n",         __func__, priv->irq_hpd_num);
	hwlog_info("%s: irq_short_num=%d\n",       __func__, priv->irq_short_num);
	hwlog_info("%s: dp_in_num=%d\n",           __func__, priv->dp_in_num);
	hwlog_info("%s: dp_out_num=%d\n",          __func__, priv->dp_out_num);
	hwlog_info("%s: link_retraining_num=%d\n", __func__, priv->link_retraining_num);
#endif

	count = MIN(priv->irq_hpd_num, DP_DSM_TCA_DEV_TYPE_NUM);
	dp_data_bin_to_str(dev_type, DP_DSM_TCA_DEV_TYPE_NUM + 1, priv->tca_dev_type, count, DP_DSM_BIN_TO_STR_SHIFT8);
#ifdef DP_DSM_DEBUG
	hwlog_info("%s: tca_dev_type=%s\n", __func__, dev_type);
#endif

	count = MIN(priv->irq_vector_num, DP_DSM_IRQ_VECTOR_NUM);
	dp_data_bin_to_str(irq_vector, DP_DSM_IRQ_VECTOR_NUM + 1, priv->irq_vector, count, DP_DSM_BIN_TO_STR_SHIFT8);
#ifdef DP_DSM_DEBUG
	hwlog_info("%s: irq_vector_num=%d\n", __func__, priv->irq_vector_num);
	hwlog_info("%s: irq_vector=%s\n",     __func__, irq_vector);
#endif

	count = MIN(priv->source_switch_num, DP_DSM_SOURCE_SWITCH_NUM);
	dp_data_bin_to_str(source_switch, DP_DSM_SOURCE_SWITCH_NUM + 1, priv->source_switch, count, DP_DSM_BIN_TO_STR_SHIFT8);
#ifdef DP_DSM_DEBUG
	hwlog_info("%s: source_switch_num=%d\n", __func__, priv->source_switch_num);
	hwlog_info("%s: source_switch=%s\n",     __func__, source_switch);
#endif

	ret += imonitor_set_param_string_v2(obj, "InTime",  in_time);
	ret += imonitor_set_param_string_v2(obj, "OutTime", out_time);

	ret += imonitor_set_param_integer_v2(obj, "EvtNum",    (long)priv->event_num);
	ret += imonitor_set_param_integer_v2(obj, "CableType", (long)priv->tpyec_cable_type);
	ret += imonitor_set_param_integer_v2(obj, "HpdNum",    (long)priv->irq_hpd_num);
	ret += imonitor_set_param_integer_v2(obj, "IrqNum",    (long)priv->irq_short_num);
	ret += imonitor_set_param_integer_v2(obj, "DpInNum",   (long)priv->dp_in_num);
	ret += imonitor_set_param_integer_v2(obj, "DpOutNum",  (long)priv->dp_out_num);
	ret += imonitor_set_param_integer_v2(obj, "LinkNum",   (long)priv->link_retraining_num);
	ret += imonitor_set_param_string_v2(obj,  "DevType",   dev_type);

	ret += imonitor_set_param_integer_v2(obj, "IrqVecNum", (long)priv->irq_vector_num);
	ret += imonitor_set_param_string_v2(obj,  "IrqVec",    irq_vector);
	ret += imonitor_set_param_integer_v2(obj, "SwNum",     (long)priv->source_switch_num);
	ret += imonitor_set_param_string_v2(obj,  "SrcSw",     source_switch);

	return ret;
}

static int dp_imonitor_prepare_link_training_info(struct imonitor_eventobj *obj, void *data)
{
	dp_dsm_priv_t *priv = DP_DSM_IMONITOR_PREPARE_PRIV;
	dp_hotplug_info_t *hotplug = NULL;
	int ret = 0;

	DP_DSM_CHECK_HOTPLUG_PTR_R(priv, obj);
	if ((0 == hotplug->link_training_retval) && (priv->link_retraining_retval < 0)) {
		hotplug->link_training_retval = priv->link_retraining_retval + (-DP_DSM_ERRNO_DEVICE_SRS_FAILED);
	}

#ifdef DP_DSM_DEBUG
	hwlog_info("%s: tpyec_cable_type=%u\n", __func__, priv->tpyec_cable_type);

	hwlog_info("%s: mid=%s\n",         __func__, hotplug->mid);
	hwlog_info("%s: monitor=%s\n",     __func__, hotplug->monitor);
	hwlog_info("%s: max_width=%u\n",   __func__, hotplug->max_width);
	hwlog_info("%s: max_high=%u\n",    __func__, hotplug->max_high);
	hwlog_info("%s: pixel_clock=%d\n", __func__, hotplug->pixel_clock);
	hwlog_info("%s: max_rate=%u\n",    __func__, hotplug->max_rate);
	hwlog_info("%s: max_lanes=%u\n",   __func__, hotplug->max_lanes);
	hwlog_info("%s: rate=%u\n",        __func__, hotplug->rate);
	hwlog_info("%s: lanes=%u\n",       __func__, hotplug->lanes);
	hwlog_info("%s: vs_pe=0x%08x\n",   __func__, hotplug->vp.vswing_preemp);

	hwlog_info("%s: safe_mode=%d\n",            __func__, hotplug->safe_mode);
	hwlog_info("%s: read_edid_retval=%d\n",     __func__, hotplug->read_edid_retval);
	hwlog_info("%s: link_training_retval=%d\n", __func__, hotplug->link_training_retval);

	hwlog_info("%s: aux_rw.rw=%d\n",     __func__, priv->aux_rw.rw);
	hwlog_info("%s: aux_rw.i2c=%d\n",    __func__, priv->aux_rw.i2c);
	hwlog_info("%s: aux_rw.addr=0x%x\n", __func__, priv->aux_rw.addr);
	hwlog_info("%s: aux_rw.len=%u\n",    __func__, priv->aux_rw.len);
	hwlog_info("%s: aux_rw.retval=%d\n", __func__, priv->aux_rw.retval);
#endif

	ret += imonitor_set_param_integer_v2(obj, "CableType", (long)priv->tpyec_cable_type);

	ret += imonitor_set_param_integer_v2(obj, "MaxWidth",  (long)hotplug->max_width);
	ret += imonitor_set_param_integer_v2(obj, "MaxHigh",   (long)hotplug->max_high);
	ret += imonitor_set_param_integer_v2(obj, "PixelClk",  (long)hotplug->pixel_clock);
	ret += imonitor_set_param_integer_v2(obj, "MaxRate",   (long)hotplug->max_rate);
	ret += imonitor_set_param_integer_v2(obj, "MaxLanes",  (long)hotplug->max_lanes);
	ret += imonitor_set_param_integer_v2(obj, "LinkRate",  (long)hotplug->rate);
	ret += imonitor_set_param_integer_v2(obj, "LinkLanes", (long)hotplug->lanes);
	ret += imonitor_set_param_integer_v2(obj, "VsPe",      (long)hotplug->vp.vswing_preemp);

	ret += imonitor_set_param_integer_v2(obj, "SafeMode", (long)hotplug->safe_mode);
	ret += imonitor_set_param_integer_v2(obj, "RedidRet", (long)hotplug->read_edid_retval);
	ret += imonitor_set_param_integer_v2(obj, "LinkRet",  (long)hotplug->link_training_retval);

	ret += imonitor_set_param_integer_v2(obj, "AuxRw",   (long)priv->aux_rw.rw);
	ret += imonitor_set_param_integer_v2(obj, "AuxI2c",  (long)priv->aux_rw.i2c);
	ret += imonitor_set_param_integer_v2(obj, "AuxAddr", (long)priv->aux_rw.addr);
	ret += imonitor_set_param_integer_v2(obj, "AuxLen",  (long)priv->aux_rw.len);
	ret += imonitor_set_param_integer_v2(obj, "AuxRet",  (long)priv->aux_rw.retval);

	return ret;
}

static int dp_imonitor_prepare_hotplug_info(struct imonitor_eventobj *obj, void *data)
{
	dp_dsm_priv_t *priv = DP_DSM_IMONITOR_PREPARE_PRIV;
	int ret = 0;

	if ((NULL == priv) || (NULL == obj)) {
		hwlog_err("%s: priv or obj is NULL!!!\n", __func__);
		return -EINVAL;
	}

#ifndef DP_DSM_DEBUG
	if (!(priv->aux_rw.retval < 0 || priv->hotplug_retval <= -DP_DSM_ERRNO_DSS_PWRON_FAILED)) { // unkown error
		return -EFAULT;
	}
#else
	// priv->aux_rw.retval < 0
	// edid/dpcd read or write error
	hwlog_info("%s: hotplug_retval=%d\n", __func__, priv->hotplug_retval);
	hwlog_info("%s: aux_rw.rw=%d\n",      __func__, priv->aux_rw.rw);
	hwlog_info("%s: aux_rw.i2c=%d\n",     __func__, priv->aux_rw.i2c);
	hwlog_info("%s: aux_rw.addr=0x%x\n",  __func__, priv->aux_rw.addr);
	hwlog_info("%s: aux_rw.len=%u\n",     __func__, priv->aux_rw.len);
	hwlog_info("%s: aux_rw.retval=%d\n",  __func__, priv->aux_rw.retval);
#endif

	ret += imonitor_set_param_integer_v2(obj, "HpRet",   (long)priv->hotplug_retval);
	ret += imonitor_set_param_integer_v2(obj, "AuxRw",   (long)priv->aux_rw.rw);
	ret += imonitor_set_param_integer_v2(obj, "AuxI2c",  (long)priv->aux_rw.i2c);
	ret += imonitor_set_param_integer_v2(obj, "AuxAddr", (long)priv->aux_rw.addr);
	ret += imonitor_set_param_integer_v2(obj, "AuxLen",  (long)priv->aux_rw.len);
	ret += imonitor_set_param_integer_v2(obj, "AuxRet",  (long)priv->aux_rw.retval);

	return ret;
}

static int dp_imonitor_prepare_hdcp_info(struct imonitor_eventobj *obj, void *data)
{
	dp_dsm_priv_t *priv = DP_DSM_IMONITOR_PREPARE_PRIV;
	int ret = 0;

	if ((NULL == priv) || (NULL == obj)) {
		hwlog_err("%s: priv or obj is NULL!!!\n", __func__);
		return -EINVAL;
	}

#ifdef DP_DSM_DEBUG
	hwlog_info("%s: hdcp_version=%u\n", __func__, priv->hdcp_version);
	hwlog_info("%s: hdcp_key=%u\n",     __func__, priv->hdcp_key);
#endif

	ret += imonitor_set_param_integer_v2(obj, "HdcpVer", (long)priv->hdcp_version);
	ret += imonitor_set_param_integer_v2(obj, "HdcpKey", (long)priv->hdcp_key);

	return ret;
}

static unsigned int dp_imonitor_get_event_id(dp_imonitor_type_t type, imonitor_prepare_param_cb_t *prepare)
{
	int count = ARRAY_SIZE(imonitor_event_id);
	unsigned int event_id = 0;
	int i = 0;

	if (type >= DP_IMONITOR_TYPE_NUM) {
		goto err_out;
	}

	for (i = 0; i < count; i++) {
		if (imonitor_event_id[i].type == type) {
			event_id = imonitor_event_id[i].event_id;
			*prepare = imonitor_event_id[i].prepare_cb;
			break;
		}
	}

err_out:
	return event_id;
}

void dp_imonitor_report(dp_imonitor_type_t type, void *data)
{
	struct imonitor_eventobj *obj = NULL;
	imonitor_prepare_param_cb_t prepare = NULL;
	unsigned int event_id = 0;
	int ret = 0;

	hwlog_info("%s enter...\n", __func__);
	event_id = dp_imonitor_get_event_id(type, &prepare);
	if ((0 == event_id) || (NULL == prepare)) {
		hwlog_err("%s: invalid type %d, event_id %u or prepare %pK!!!\n", __func__, type, event_id, prepare);
		return;
	}

	obj = imonitor_create_eventobj(event_id);
	if (NULL == obj) {
		hwlog_err("%s: imonitor_create_eventobj %u failed!!!\n", __func__, event_id);
		return;
	}

	ret = prepare(obj, data);
	if (ret < 0) {
		hwlog_info("%s: prepare param %u skip(%d)!!!\n", __func__, event_id, ret);
		goto err_out;
	}

	ret = imonitor_send_event(obj);
	if (ret < 0) {
		hwlog_err("%s: imonitor_send_event %u failed %d!!!\n", __func__, event_id, ret);
		goto err_out;
	}
	hwlog_info("%s event_id %u success.\n", __func__, event_id);

err_out:
	if (obj != NULL) {
		imonitor_destroy_eventobj(obj);
	}
}
EXPORT_SYMBOL_GPL(dp_imonitor_report);

static int dp_dmd_get_error_no(dp_dmd_type_t type)
{
	int count = ARRAY_SIZE(dmd_error_no);
	int error_no = 0;
	int i = 0;

	if (type >= DP_DMD_TYPE_NUM) {
		goto err_out;
	}

	for (i = 0; i < count; i++) {
		if (dmd_error_no[i].type == type) {
			error_no = dmd_error_no[i].error_no;
			break;
		}
	}

err_out:
	return error_no;
}

void dp_dmd_report(dp_dmd_type_t type, const char *fmt, ...)
{
	struct dsm_client *client = g_dp_dsm_client;
	char report_buf[DP_DSM_BUF_SIZE] = {0};
	va_list args;
	int error_no = 0;
	int ret = 0;

	hwlog_info("%s enter...\n", __func__);
	if ((NULL == client) || (NULL == fmt)) {
		hwlog_err("%s: client or fmt is NULL!!!\n", __func__);
		return;
	}

	error_no = dp_dmd_get_error_no(type);
	if (error_no <= 0) {
		hwlog_err("%s: invalid type %d or error_no %d!!!\n", __func__, type, error_no);
		return;
	}

	va_start(args, fmt);
	ret = vsnprintf(report_buf, sizeof(report_buf) - 1, fmt, args);
	va_end(args);
	if (ret < 0) {
		hwlog_err("%s: vsnprintf failed %d!!!\n", __func__, ret);
		return;
	}

	ret = dsm_client_ocuppy(client);
	if (!ret) {
		ret = dsm_client_record(client, report_buf);
		dsm_client_notify(client, error_no);
		hwlog_info("%s: report %d success(%d)\n", __func__, error_no, ret);
	} else {
		hwlog_info("%s: report %d skip(%d)\n", __func__, error_no, ret);
	}
}
EXPORT_SYMBOL_GPL(dp_dmd_report);

static void dp_dsm_release(dp_dsm_priv_t *priv, struct dsm_client *client)
{
	cancel_delayed_work_sync(&(g_imonitor_report.imonitor_work));
	cancel_delayed_work_sync(&(g_imonitor_report.hpd_work));

	if (client != NULL) {
		dsm_unregister_client(client, &dp_dsm);
	}

	if (priv != NULL) {
		dp_clear_priv_data(priv);
		kfree(priv);
	}
}

static int __init dp_dsm_init(void)
{
	dp_dsm_priv_t *priv = NULL;
	struct dsm_client *client = NULL;
	int ret = 0;

	hwlog_info("%s: enter...\n", __func__);
	client = dsm_register_client(&dp_dsm);
	if (NULL == client) {
		client = dsm_find_client((char *)dp_dsm.name);
		if (NULL == client) {
			hwlog_err("%s: dsm_register_client failed!!!\n", __func__);
			ret = -EFAULT;
			goto err_out;
		} else {
			hwlog_info("%s: dsm_find_client %pK!!!\n", __func__, client);
		}
	}

	priv = (dp_dsm_priv_t *)kzalloc(sizeof(dp_dsm_priv_t), GFP_KERNEL);
	if (NULL == priv) {
		hwlog_err("%s: kzalloc priv failed!!!\n", __func__);
		ret = -ENOMEM;
		goto err_out;
	}

	dp_reinit_priv_data(priv);
#ifdef DP_DEBUG_ENABLE
	memset(&g_typec_in_out, 0, sizeof(dp_typec_in_out_t));
#endif

	// init report info
	memset(&g_imonitor_report, 0, sizeof(dp_imonitor_report_info_t));
	INIT_LIST_HEAD(&(g_imonitor_report.list_head));
	mutex_init(&(g_imonitor_report.lock));
	INIT_DELAYED_WORK(&(g_imonitor_report.imonitor_work), dp_imonitor_report_info_work);
	INIT_DELAYED_WORK(&(g_imonitor_report.hpd_work), dp_hpd_check_work);
	g_imonitor_report.hpd_jiffies = jiffies + msecs_to_jiffies(DP_DSM_HPD_TIME_INTERVAL);
	g_imonitor_report.report_jiffies = jiffies + msecs_to_jiffies(DP_DSM_REPORT_TIME_INTERVAL);

	g_dp_dsm_priv   = priv;
	g_dp_dsm_client = client;
	hwlog_info("%s: init success!!!\n", __func__);
	return 0;

err_out:
	dp_dsm_release(priv, client);
    return ret;
}

static void __exit dp_dsm_exit(void)
{
	hwlog_info("%s: enter...\n", __func__);
	dp_dsm_release(g_dp_dsm_priv, g_dp_dsm_client);
	g_dp_dsm_priv = NULL;
	g_dp_dsm_client = NULL;
}

module_init(dp_dsm_init);
module_exit(dp_dsm_exit);

MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("Huawei dp dsm driver");
MODULE_AUTHOR("<wangping48@huawei.com>");
#endif // DP_DSM_ENABLE

