/*Add synaptics new driver "Synaptics DSX I2C V2.0"*/
/*
 * Synaptics DSX touchscreen driver
 *
 * Copyright (C) 2012 Synaptics Incorporated
 *
 * Copyright (C) 2012 Alexandra Chin <alexandra.chin@tw.synaptics.com>
 * Copyright (C) 2012 Scott Lin <scott.lin@tw.synaptics.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/slab.h>
#include <linux/i2c.h>
#include <linux/interrupt.h>
#include <linux/delay.h>
#include <linux/input.h>
#include <linux/ctype.h>
#include <linux/hrtimer.h>
#include "synaptics.h"
#include "../../huawei_ts_kit.h"
#include <linux/gpio.h>
#include <linux/regulator/consumer.h>

#define WATCHDOG_HRTIMER
#define WATCHDOG_TIMEOUT_S 2
#define FORCE_TIMEOUT_100MS 10
#define MAX_I2C_MSG_LENS 0x3F
/*#define STATUS_WORK_INTERVAL 20  ms */
/*Add synaptics capacitor test function */
#define MMITEST
/*
#define RAW_HEX
#define HUMAN_READABLE
*/
#define F54_MAX_CAP_TITLE_SIZE	50
#define DATA_TEMP_BUFF_SIZE	8
#define IRQ_ON 1
#define IRQ_OFF 0
#define Interrupt_Enable_Addr  0x0052
#define IRQ_ENA_MASK_BIT 0x04
#define STATUS_IDLE 0
#define STATUS_BUSY 1

#define DATA_REPORT_INDEX_OFFSET 1
#define DATA_REPORT_DATA_OFFSET 3

#define SENSOR_RX_MAPPING_OFFSET 1
#define SENSOR_TX_MAPPING_OFFSET 2

#define COMMAND_GET_REPORT 1
#define COMMAND_FORCE_CAL 2
#define COMMAND_FORCE_UPDATE 4

#define CONTROL_42_SIZE 2
#define CONTROL_43_54_SIZE 13
#define CONTROL_55_56_SIZE 2
#define CONTROL_58_SIZE 1
#define CONTROL_59_SIZE 2
#define CONTROL_60_62_SIZE 3
#define CONTROL_63_SIZE 1
#define CONTROL_64_67_SIZE 4
#define CONTROL_68_73_SIZE 8
#define CONTROL_74_SIZE 2
#define CONTROL_76_SIZE 1
#define CONTROL_77_78_SIZE 2
#define CONTROL_79_83_SIZE 5
#define CONTROL_84_85_SIZE 2
#define CONTROL_86_SIZE 1
#define CONTROL_87_SIZE 1

#define HIGH_RESISTANCE_DATA_SIZE 6
#define FULL_RAW_CAP_MIN_MAX_DATA_SIZE 4
#define TREX_DATA_SIZE 7
#define NO_AUTO_CAL_MASK 0x01

#define concat(a, b) a##b

#define GROUP(_attrs) {\
	.attrs = _attrs,\
}

#define attrify(propname) (&dev_attr_##propname.attr)

#define show_prototype(propname)\
static ssize_t concat(synaptics_rmi4_f54, _##propname##_show)(\
		struct device *dev,\
		struct device_attribute *attr,\
		char *buf);\
\
static struct device_attribute dev_attr_##propname =\
		__ATTR(propname, S_IRUGO,\
		concat(synaptics_rmi4_f54, _##propname##_show),\
		NULL);

#define store_prototype(propname)\
static ssize_t concat(synaptics_rmi4_f54, _##propname##_store)(\
		struct device *dev,\
		struct device_attribute *attr,\
		const char *buf, size_t count);\
\
static struct device_attribute dev_attr_##propname =\
		__ATTR(propname, (S_IWUSR | S_IWGRP),\
		NULL,\
		concat(synaptics_rmi4_f54, _##propname##_store));

#define show_store_prototype(propname)\
static ssize_t concat(synaptics_rmi4_f54, _##propname##_show)(\
		struct device *dev,\
		struct device_attribute *attr,\
		char *buf);\
\
static ssize_t concat(synaptics_rmi4_f54, _##propname##_store)(\
		struct device *dev,\
		struct device_attribute *attr,\
		const char *buf, size_t count);\
\
static struct device_attribute dev_attr_##propname =\
		__ATTR(propname, (S_IRUGO | S_IWUSR | S_IWGRP),\
		concat(synaptics_rmi4_f54, _##propname##_show),\
		concat(synaptics_rmi4_f54, _##propname##_store));

#define simple_show_func(rtype, propname, fmt)\
static ssize_t concat(synaptics_rmi4_f54, _##propname##_show)(\
		struct device *dev,\
		struct device_attribute *attr,\
		char *buf)\
{\
	return snprintf(buf, PAGE_SIZE, fmt, f54->rtype.propname);\
} \

#define simple_show_func_unsigned(rtype, propname)\
simple_show_func(rtype, propname, "%u\n")

#define show_func(rtype, rgrp, propname, fmt)\
static ssize_t concat(synaptics_rmi4_f54, _##propname##_show)(\
		struct device *dev,\
		struct device_attribute *attr,\
		char *buf)\
{\
	int retval;\
	struct synaptics_rmi4_data *rmi4_data = f54->rmi4_data;\
\
	mutex_lock(&f54->rtype##_mutex);\
\
	retval = f54->fn_ptr->read(rmi4_data,\
			f54->rtype.rgrp->address,\
			f54->rtype.rgrp->data,\
			sizeof(f54->rtype.rgrp->data));\
	mutex_unlock(&f54->rtype##_mutex);\
	if (retval < 0) {\
		dev_err(&rmi4_data->i2c_client->dev,\
				"%s: Failed to read " #rtype\
				" " #rgrp "\n",\
				__func__);\
		return retval;\
	} \
\
	return snprintf(buf, PAGE_SIZE, fmt,\
			f54->rtype.rgrp->propname);\
} \

#define show_store_func(rtype, rgrp, propname, fmt)\
show_func(rtype, rgrp, propname, fmt)\
\
static ssize_t concat(synaptics_rmi4_f54, _##propname##_store)(\
		struct device *dev,\
		struct device_attribute *attr,\
		const char *buf, size_t count)\
{\
	int retval;\
	unsigned long setting;\
	unsigned long o_setting;\
	struct synaptics_rmi4_data *rmi4_data = f54->rmi4_data;\
\
	retval = sstrtoul(buf, 10, &setting);\
	if (retval)\
		return retval;\
\
	mutex_lock(&f54->rtype##_mutex);\
	retval = f54->fn_ptr->read(rmi4_data,\
			f54->rtype.rgrp->address,\
			f54->rtype.rgrp->data,\
			sizeof(f54->rtype.rgrp->data));\
	if (retval < 0) {\
		mutex_unlock(&f54->rtype##_mutex);\
		dev_err(&rmi4_data->i2c_client->dev,\
				"%s: Failed to read " #rtype\
				" " #rgrp "\n",\
				__func__);\
		return retval;\
	} \
\
	if (f54->rtype.rgrp->propname == setting) {\
		mutex_unlock(&f54->rtype##_mutex);\
		return count;\
	} \
\
	o_setting = f54->rtype.rgrp->propname;\
	f54->rtype.rgrp->propname = setting;\
\
	retval = f54->fn_ptr->write(rmi4_data,\
			f54->rtype.rgrp->address,\
			f54->rtype.rgrp->data,\
			sizeof(f54->rtype.rgrp->data));\
	if (retval < 0) {\
		dev_err(&rmi4_data->i2c_client->dev,\
				"%s: Failed to write " #rtype\
				" " #rgrp "\n",\
				__func__);\
		f54->rtype.rgrp->propname = o_setting;\
		mutex_unlock(&f54->rtype##_mutex);\
		return retval;\
	} \
\
	mutex_unlock(&f54->rtype##_mutex);\
	return count;\
} \

#define show_store_func_unsigned(rtype, rgrp, propname)\
show_store_func(rtype, rgrp, propname, "%u\n")

#define show_replicated_func(rtype, rgrp, propname, fmt)\
static ssize_t concat(synaptics_rmi4_f54, _##propname##_show)(\
		struct device *dev,\
		struct device_attribute *attr,\
		char *buf)\
{\
	int retval;\
	int size = 0;\
	unsigned char ii;\
	unsigned char length;\
	unsigned char *temp;\
	struct synaptics_rmi4_data *rmi4_data = f54->rmi4_data;\
\
	mutex_lock(&f54->rtype##_mutex);\
\
	length = f54->rtype.rgrp->length;\
\
	retval = f54->fn_ptr->read(rmi4_data,\
			f54->rtype.rgrp->address,\
			(unsigned char *)f54->rtype.rgrp->data,\
			length);\
	mutex_unlock(&f54->rtype##_mutex);\
	if (retval < 0) {\
		dev_dbg(&rmi4_data->i2c_client->dev,\
				"%s: Failed to read " #rtype\
				" " #rgrp "\n",\
				__func__);\
	} \
\
	temp = buf;\
\
	for (ii = 0; ii < length; ii++) {\
		retval = snprintf(temp, PAGE_SIZE - size, fmt " ",\
				f54->rtype.rgrp->data[ii].propname);\
		if (retval < 0) {\
			dev_err(&rmi4_data->i2c_client->dev,\
					"%s: Faild to write output\n",\
					__func__);\
			return retval;\
		} \
		size += retval;\
		temp += retval;\
	} \
\
	retval = snprintf(temp, PAGE_SIZE - size, "\n");\
	if (retval < 0) {\
		dev_err(&rmi4_data->i2c_client->dev,\
				"%s: Faild to write null terminator\n",\
				__func__);\
		return retval;\
	} \
\
	return size + retval;\
} \

#define show_replicated_func_unsigned(rtype, rgrp, propname)\
show_replicated_func(rtype, rgrp, propname, "%u")

#define show_store_replicated_func(rtype, rgrp, propname, fmt)\
show_replicated_func(rtype, rgrp, propname, fmt)\
\
static ssize_t concat(synaptics_rmi4_f54, _##propname##_store)(\
		struct device *dev,\
		struct device_attribute *attr,\
		const char *buf, size_t count)\
{\
	int retval;\
	unsigned int setting;\
	unsigned char ii;\
	unsigned char length;\
	const unsigned char *temp;\
	struct synaptics_rmi4_data *rmi4_data = f54->rmi4_data;\
\
	mutex_lock(&f54->rtype##_mutex);\
\
	length = f54->rtype.rgrp->length;\
\
	retval = f54->fn_ptr->read(rmi4_data,\
			f54->rtype.rgrp->address,\
			(unsigned char *)f54->rtype.rgrp->data,\
			length);\
	if (retval < 0) {\
		dev_dbg(&rmi4_data->i2c_client->dev,\
				"%s: Failed to read " #rtype\
				" " #rgrp "\n",\
				__func__);\
	} \
\
	temp = buf;\
\
	for (ii = 0; ii < length; ii++) {\
		if (sscanf(temp, fmt, &setting) == 1) {\
			f54->rtype.rgrp->data[ii].propname = setting;\
		} else {\
			retval = f54->fn_ptr->read(rmi4_data,\
					f54->rtype.rgrp->address,\
					(unsigned char *)f54->rtype.rgrp->data,\
					length);\
			mutex_unlock(&f54->rtype##_mutex);\
			return -EINVAL;\
		} \
\
		while (*temp != 0) {\
			temp++;\
			if (isspace(*(temp - 1)) && !isspace(*temp))\
				break;\
		} \
	} \
\
	retval = f54->fn_ptr->write(rmi4_data,\
			f54->rtype.rgrp->address,\
			(unsigned char *)f54->rtype.rgrp->data,\
			length);\
	mutex_unlock(&f54->rtype##_mutex);\
	if (retval < 0) {\
		dev_err(&rmi4_data->i2c_client->dev,\
				"%s: Failed to write " #rtype\
				" " #rgrp "\n",\
				__func__);\
		return retval;\
	} \
\
	return count;\
} \

#define show_store_replicated_func_unsigned(rtype, rgrp, propname)\
show_store_replicated_func(rtype, rgrp, propname, "%u")

enum f54_report_types {
	F54_8BIT_IMAGE = 1,
	F54_16BIT_IMAGE = 2,
	F54_RAW_16BIT_IMAGE = 3,
	F54_HIGH_RESISTANCE = 4,
	F54_TX_TO_TX_SHORT = 5,
	F54_RX_TO_RX1 = 7,
	F54_TRUE_BASELINE = 9,
	F54_FULL_RAW_CAP_MIN_MAX = 13,
	F54_RX_OPENS1 = 14,
	F54_TX_OPEN = 15,
	F54_TX_TO_GROUND = 16,
	F54_RX_TO_RX2 = 17,
	F54_RX_OPENS2 = 18,
	F54_FULL_RAW_CAP = 19,
	F54_FULL_RAW_CAP_RX_COUPLING_COMP = 20,
	F54_SENSOR_SPEED = 22,
	F54_ADC_RANGE = 23,
	F54_TREX_OPENS = 24,
	F54_TREX_TO_GND = 25,
	F54_TREX_SHORTS = 26,
	INVALID_REPORT_TYPE = -1,
};

extern struct synaptics_rmi4_data *rmi4_data;
/*Add synaptics capacitor test function */
#ifdef MMITEST
/*#define CFG_F54_TXCOUNT       14*/
/*#define CFG_F54_RXCOUNT       25*/
#define TOUCH_RX_DEFAULT 26
#define TOUCH_TX_DEFAULT 14
#define TP_TEST_FAILED_REASON_LEN 20

static unsigned char rx = TOUCH_RX_DEFAULT;
static unsigned char tx = TOUCH_TX_DEFAULT;

static bool sysfs_is_busy;

static short FullRawMaxCap;
static short FullRawMinCap;

static short *FullRawCapUpperLimit;
static short *FullRawCapLowerLimit;

static short *HighResistanceUpperLimit;
static short *HighResistanceLowerLimit;

static int RxDiagonalUpperLimit;
static int RxDiagonalLowerLimit;


static int RxOthersUpperLimit = 250;
static char TxTxReportLimit;
enum mmi_results {
	TEST_FAILED,
	TEST_PASS,
};
static char tp_test_failed_reason[TP_TEST_FAILED_REASON_LEN] = { "-software_reason" };
static char *g_mmi_buf_f54test_result;
static char *g_mmi_highresistance_report;
static char *g_mmi_maxmincapacitance_report;
static char *g_mmi_RxtoRxshort_report;
static char *g_mmi_buf_f54raw_data;
static char *g_buf_debug_data;

static char g_synaptics_trigger_log_flag;
#endif

static int synaptics_rmi4_f54_attention(void);

struct f54_query {
	union {
		struct {
			/* query 0 */
			unsigned char num_of_rx_electrodes;

			/* query 1 */
			unsigned char num_of_tx_electrodes;

			/* query 2 */
			unsigned char f54_query2_b0__1:2;
			unsigned char has_baseline:1;
			unsigned char has_image8:1;
			unsigned char f54_query2_b4__5:2;
			unsigned char has_image16:1;
			unsigned char f54_query2_b7:1;

			/* queries 3.0 and 3.1 */
			unsigned short clock_rate;

			/* query 4 */
			unsigned char touch_controller_family;

			/* query 5 */
			unsigned char has_pixel_touch_threshold_adjustment:1;
			unsigned char f54_query5_b1__7:7;

			/* query 6 */
			unsigned char has_sensor_assignment:1;
			unsigned char has_interference_metric:1;
			unsigned char has_sense_frequency_control:1;
			unsigned char has_firmware_noise_mitigation:1;
			unsigned char has_ctrl11:1;
			unsigned char has_two_byte_report_rate:1;
			unsigned char has_one_byte_report_rate:1;
			unsigned char has_relaxation_control:1;

			/* query 7 */
			unsigned char curve_compensation_mode:2;
			unsigned char f54_query7_b2__7:6;

			/* query 8 */
			unsigned char f54_query8_b0:1;
			unsigned char has_iir_filter:1;
			unsigned char has_cmn_removal:1;
			unsigned char has_cmn_maximum:1;
			unsigned char has_touch_hysteresis:1;
			unsigned char has_edge_compensation:1;
			unsigned char has_per_frequency_noise_control:1;
			unsigned char has_enhanced_stretch:1;

			/* query 9 */
			unsigned char has_force_fast_relaxation:1;
			unsigned char has_multi_metric_state_machine:1;
			unsigned char has_signal_clarity:1;
			unsigned char has_variance_metric:1;
			unsigned char has_0d_relaxation_control:1;
			unsigned char has_0d_acquisition_control:1;
			unsigned char has_status:1;
			unsigned char has_slew_metric:1;

			/* query 10 */
			unsigned char has_h_blank:1;
			unsigned char has_v_blank:1;
			unsigned char has_long_h_blank:1;
			unsigned char has_startup_fast_relaxation:1;
			unsigned char has_esd_control:1;
			unsigned char has_noise_mitigation2:1;
			unsigned char has_noise_state:1;
			unsigned char has_energy_ratio_relaxation:1;

			/* query 11 */
			unsigned char has_excessive_noise_reporting:1;
			unsigned char has_slew_option:1;
			unsigned char has_two_overhead_bursts:1;
			unsigned char has_query13:1;
			unsigned char has_one_overhead_burst:1;
			unsigned char f54_query11_b5:1;
			unsigned char has_ctrl88:1;
			unsigned char has_query15:1;

			/* query 12 */
			unsigned char number_of_sensing_frequencies:4;
			unsigned char f54_query12_b4__7:4;

			/* query 13 */
			unsigned char has_ctrl86:1;
			unsigned char has_ctrl87:1;
			unsigned char has_ctrl87_sub0:1;
			unsigned char has_ctrl87_sub1:1;
			unsigned char has_ctrl87_sub2:1;
			unsigned char has_cidim:1;
			unsigned char has_noise_mitigation_enhancement:1;
			unsigned char has_rail_im:1;
		} __packed;
		unsigned char data[15];
	};
};

struct f54_control_0 {
	union {
		struct {
			unsigned char no_relax:1;
			unsigned char no_scan:1;
			unsigned char force_fast_relaxation:1;
			unsigned char startup_fast_relaxation:1;
			unsigned char gesture_cancels_sfr:1;
			unsigned char enable_energy_ratio_relaxation:1;
			unsigned char excessive_noise_attn_enable:1;
			unsigned char f54_control0_b7:1;
		} __packed;
		struct {
			unsigned char data[1];
			unsigned short address;
		} __packed;
	};
};

struct f54_control_1 {
	union {
		struct {
			unsigned char bursts_per_cluster:4;
			unsigned char f54_ctrl1_b4__7:4;
		} __packed;
		struct {
			unsigned char data[1];
			unsigned short address;
		} __packed;
	};
};

struct f54_control_2 {
	union {
		struct {
			unsigned short saturation_cap;
		} __packed;
		struct {
			unsigned char data[2];
			unsigned short address;
		} __packed;
	};
};

struct f54_control_3 {
	union {
		struct {
			unsigned char pixel_touch_threshold;
		} __packed;
		struct {
			unsigned char data[1];
			unsigned short address;
		} __packed;
	};
};

struct f54_control_4__6 {
	union {
		struct {
			/* control 4 */
			unsigned char rx_feedback_cap:2;
			unsigned char bias_current:2;
			unsigned char f54_ctrl4_b4__7:4;

			/* control 5 */
			unsigned char low_ref_cap:2;
			unsigned char low_ref_feedback_cap:2;
			unsigned char low_ref_polarity:1;
			unsigned char f54_ctrl5_b5__7:3;

			/* control 6 */
			unsigned char high_ref_cap:2;
			unsigned char high_ref_feedback_cap:2;
			unsigned char high_ref_polarity:1;
			unsigned char f54_ctrl6_b5__7:3;
		} __packed;
		struct {
			unsigned char data[3];
			unsigned short address;
		} __packed;
	};
};

struct f54_control_7 {
	union {
		struct {
			unsigned char cbc_cap:3;
			unsigned char cbc_polarity:1;
			unsigned char cbc_tx_carrier_selection:1;
			unsigned char f54_ctrl7_b5__7:3;
		} __packed;
		struct {
			unsigned char data[1];
			unsigned short address;
		} __packed;
	};
};

struct f54_control_8__9 {
	union {
		struct {
			/* control 8 */
			unsigned short integration_duration:10;
			unsigned short f54_ctrl8_b10__15:6;

			/* control 9 */
			unsigned char reset_duration;
		} __packed;
		struct {
			unsigned char data[3];
			unsigned short address;
		} __packed;
	};
};

struct f54_control_10 {
	union {
		struct {
			unsigned char noise_sensing_bursts_per_image:4;
			unsigned char f54_ctrl10_b4__7:4;
		} __packed;
		struct {
			unsigned char data[1];
			unsigned short address;
		} __packed;
	};
};

struct f54_control_11 {
	union {
		struct {
			unsigned short f54_ctrl11;
		} __packed;
		struct {
			unsigned char data[2];
			unsigned short address;
		} __packed;
	};
};

struct f54_control_12__13 {
	union {
		struct {
			/* control 12 */
			unsigned char slow_relaxation_rate;

			/* control 13 */
			unsigned char fast_relaxation_rate;
		} __packed;
		struct {
			unsigned char data[2];
			unsigned short address;
		} __packed;
	};
};

struct f54_control_14 {
	union {
		struct {
			unsigned char rxs_on_xaxis:1;
			unsigned char curve_comp_on_txs:1;
			unsigned char f54_ctrl14_b2__7:6;
		} __packed;
		struct {
			unsigned char data[1];
			unsigned short address;
		} __packed;
	};
};

struct f54_control_15n {
	unsigned char sensor_rx_assignment;
};

struct f54_control_15 {
	struct f54_control_15n *data;
	unsigned short address;
	unsigned char length;
};

struct f54_control_16n {
	unsigned char sensor_tx_assignment;
};

struct f54_control_16 {
	struct f54_control_16n *data;
	unsigned short address;
	unsigned char length;
};

struct f54_control_17n {
	unsigned char burst_count_b8__10:3;
	unsigned char disable:1;
	unsigned char f54_ctrl17_b4:1;
	unsigned char filter_bandwidth:3;
};

struct f54_control_17 {
	struct f54_control_17n *data;
	unsigned short address;
	unsigned char length;
};

struct f54_control_18n {
	unsigned char burst_count_b0__7;
};

struct f54_control_18 {
	struct f54_control_18n *data;
	unsigned short address;
	unsigned char length;
};

struct f54_control_19n {
	unsigned char stretch_duration;
};

struct f54_control_19 {
	struct f54_control_19n *data;
	unsigned short address;
	unsigned char length;
};

struct f54_control_20 {
	union {
		struct {
			unsigned char disable_noise_mitigation:1;
			unsigned char f54_ctrl20_b1__7:7;
		} __packed;
		struct {
			unsigned char data[1];
			unsigned short address;
		} __packed;
	};
};

struct f54_control_21 {
	union {
		struct {
			unsigned short freq_shift_noise_threshold;
		} __packed;
		struct {
			unsigned char data[2];
			unsigned short address;
		} __packed;
	};
};

struct f54_control_22__26 {
	union {
		struct {
			/* control 22 */
			unsigned char f54_ctrl22;

			/* control 23 */
			unsigned short medium_noise_threshold;

			/* control 24 */
			unsigned short high_noise_threshold;

			/* control 25 */
			unsigned char noise_density;

			/* control 26 */
			unsigned char frame_count;
		} __packed;
		struct {
			unsigned char data[7];
			unsigned short address;
		} __packed;
	};
};

struct f54_control_27 {
	union {
		struct {
			unsigned char iir_filter_coef;
		} __packed;
		struct {
			unsigned char data[1];
			unsigned short address;
		} __packed;
	};
};

struct f54_control_28 {
	union {
		struct {
			unsigned short quiet_threshold;
		} __packed;
		struct {
			unsigned char data[2];
			unsigned short address;
		} __packed;
	};
};

struct f54_control_29 {
	union {
		struct {
			/* control 29 */
			unsigned char f54_ctrl29_b0__6:7;
			unsigned char cmn_filter_disable:1;
		} __packed;
		struct {
			unsigned char data[1];
			unsigned short address;
		} __packed;
	};
};

struct f54_control_30 {
	union {
		struct {
			unsigned char cmn_filter_max;
		} __packed;
		struct {
			unsigned char data[1];
			unsigned short address;
		} __packed;
	};
};

struct f54_control_31 {
	union {
		struct {
			unsigned char touch_hysteresis;
		} __packed;
		struct {
			unsigned char data[1];
			unsigned short address;
		} __packed;
	};
};

struct f54_control_32__35 {
	union {
		struct {
			/* control 32 */
			unsigned short rx_low_edge_comp;

			/* control 33 */
			unsigned short rx_high_edge_comp;

			/* control 34 */
			unsigned short tx_low_edge_comp;

			/* control 35 */
			unsigned short tx_high_edge_comp;
		} __packed;
		struct {
			unsigned char data[8];
			unsigned short address;
		} __packed;
	};
};

struct f54_control_36n {
	unsigned char axis1_comp;
};

struct f54_control_36 {
	struct f54_control_36n *data;
	unsigned short address;
	unsigned char length;
};

struct f54_control_37n {
	unsigned char axis2_comp;
};

struct f54_control_37 {
	struct f54_control_37n *data;
	unsigned short address;
	unsigned char length;
};

struct f54_control_38n {
	unsigned char noise_control_1;
};

struct f54_control_38 {
	struct f54_control_38n *data;
	unsigned short address;
	unsigned char length;
};

struct f54_control_39n {
	unsigned char noise_control_2;
};

struct f54_control_39 {
	struct f54_control_39n *data;
	unsigned short address;
	unsigned char length;
};

struct f54_control_40n {
	unsigned char noise_control_3;
};

struct f54_control_40 {
	struct f54_control_40n *data;
	unsigned short address;
	unsigned char length;
};

struct f54_control_41 {
	union {
		struct {
			unsigned char no_signal_clarity:1;
			unsigned char f54_ctrl41_b1__7:7;
		} __packed;
		struct {
			unsigned char data[1];
			unsigned short address;
		} __packed;
	};
};

struct f54_control_57 {
	union {
		struct {
			unsigned char cbc_cap_0d:3;
			unsigned char cbc_polarity_0d:1;
			unsigned char cbc_tx_carrier_selection_0d:1;
			unsigned char f54_ctrl57_b5__7:3;
		} __packed;
		struct {
			unsigned char data[1];
			unsigned short address;
		} __packed;
	};
};

struct f54_control_88 {
	union {
		struct {
			unsigned char tx_low_reference_polarity:1;
			unsigned char tx_high_reference_polarity:1;
			unsigned char abs_low_reference_polarity:1;
			unsigned char abs_polarity:1;
			unsigned char cbc_polarity:1;
			unsigned char cbc_tx_carrier_selection:1;
			unsigned char charge_pump_enable:1;
			unsigned char cbc_abs_auto_servo:1;
		} __packed;
		struct {
			unsigned char data[1];
			unsigned short address;
		} __packed;
	};
};

struct f54_control {
	struct f54_control_0 *reg_0;
	struct f54_control_1 *reg_1;
	struct f54_control_2 *reg_2;
	struct f54_control_3 *reg_3;
	struct f54_control_4__6 *reg_4__6;
	struct f54_control_7 *reg_7;
	struct f54_control_8__9 *reg_8__9;
	struct f54_control_10 *reg_10;
	struct f54_control_11 *reg_11;
	struct f54_control_12__13 *reg_12__13;
	struct f54_control_14 *reg_14;
	struct f54_control_15 *reg_15;
	struct f54_control_16 *reg_16;
	struct f54_control_17 *reg_17;
	struct f54_control_18 *reg_18;
	struct f54_control_19 *reg_19;
	struct f54_control_20 *reg_20;
	struct f54_control_21 *reg_21;
	struct f54_control_22__26 *reg_22__26;
	struct f54_control_27 *reg_27;
	struct f54_control_28 *reg_28;
	struct f54_control_29 *reg_29;
	struct f54_control_30 *reg_30;
	struct f54_control_31 *reg_31;
	struct f54_control_32__35 *reg_32__35;
	struct f54_control_36 *reg_36;
	struct f54_control_37 *reg_37;
	struct f54_control_38 *reg_38;
	struct f54_control_39 *reg_39;
	struct f54_control_40 *reg_40;
	struct f54_control_41 *reg_41;
	struct f54_control_57 *reg_57;
	struct f54_control_88 *reg_88;
};

struct synaptics_rmi4_f54_handle {
	bool no_auto_cal;
	unsigned char status;
	unsigned char intr_mask;
	unsigned char intr_reg_num;
	unsigned char rx_assigned;
	unsigned char tx_assigned;
	unsigned char *report_data;
	unsigned short query_base_addr;
	unsigned short control_base_addr;
	unsigned short data_base_addr;
	unsigned short command_base_addr;
	unsigned short fifoindex;
	unsigned int report_size;
	unsigned int data_buffer_size;
	enum f54_report_types report_type;
	struct mutex status_mutex;
	struct mutex data_mutex;
	struct mutex control_mutex;
	struct f54_query query;
	struct f54_control control;
	struct kobject *attr_dir;
	struct hrtimer watchdog;
	struct work_struct timeout_work;
	struct delayed_work status_work;
	struct workqueue_struct *status_workqueue;
	struct synaptics_rmi4_exp_fn_ptr *fn_ptr;
	struct synaptics_rmi4_data *rmi4_data;
};

struct f55_query {
	union {
		struct {
			/* query 0 */
			unsigned char num_of_rx_electrodes;

			/* query 1 */
			unsigned char num_of_tx_electrodes;

			/* query 2 */
			unsigned char has_sensor_assignment:1;
			unsigned char has_edge_compensation:1;
			unsigned char curve_compensation_mode:2;
			unsigned char has_ctrl6:1;
			unsigned char has_alternate_transmitter_assignment:1;
			unsigned char has_single_layer_multi_touch:1;
			unsigned char has_query5:1;
		} __packed;
		unsigned char data[3];
	};
};

struct synaptics_rmi4_f55_handle {
	unsigned char *rx_assignment;
	unsigned char *tx_assignment;
	unsigned short query_base_addr;
	unsigned short control_base_addr;
	unsigned short data_base_addr;
	unsigned short command_base_addr;
	struct f55_query query;
};

show_prototype(tp_status)
    show_prototype(report_size)
    show_store_prototype(no_auto_cal)
    show_store_prototype(report_type)
    show_store_prototype(fifoindex)
    store_prototype(do_preparation)
    store_prototype(get_report)
    store_prototype(force_cal)
    store_prototype(resume_touch)
    show_prototype(num_of_mapped_rx)
    show_prototype(num_of_mapped_tx)
    show_prototype(num_of_rx_electrodes)
    show_prototype(num_of_tx_electrodes)
    show_prototype(has_image16)
    show_prototype(has_image8)
    show_prototype(has_baseline)
    show_prototype(clock_rate)
    show_prototype(touch_controller_family)
    show_prototype(has_pixel_touch_threshold_adjustment)
    show_prototype(has_sensor_assignment)
    show_prototype(has_interference_metric)
    show_prototype(has_sense_frequency_control)
    show_prototype(has_firmware_noise_mitigation)
    show_prototype(has_two_byte_report_rate)
    show_prototype(has_one_byte_report_rate)
    show_prototype(has_relaxation_control)
    show_prototype(curve_compensation_mode)
    show_prototype(has_iir_filter)
    show_prototype(has_cmn_removal)
    show_prototype(has_cmn_maximum)
    show_prototype(has_touch_hysteresis)
    show_prototype(has_edge_compensation)
    show_prototype(has_per_frequency_noise_control)
    show_prototype(has_signal_clarity)
    show_prototype(number_of_sensing_frequencies)

    show_store_prototype(no_relax)
    show_store_prototype(no_scan)
    show_store_prototype(bursts_per_cluster)
    show_store_prototype(saturation_cap)
    show_store_prototype(pixel_touch_threshold)
    show_store_prototype(rx_feedback_cap)
    show_store_prototype(low_ref_cap)
    show_store_prototype(low_ref_feedback_cap)
    show_store_prototype(low_ref_polarity)
    show_store_prototype(high_ref_cap)
    show_store_prototype(high_ref_feedback_cap)
    show_store_prototype(high_ref_polarity)
    show_store_prototype(cbc_cap)
    show_store_prototype(cbc_polarity)
    show_store_prototype(cbc_tx_carrier_selection)
    show_store_prototype(integration_duration)
    show_store_prototype(reset_duration)
    show_store_prototype(noise_sensing_bursts_per_image)
    show_store_prototype(slow_relaxation_rate)
    show_store_prototype(fast_relaxation_rate)
    show_store_prototype(rxs_on_xaxis)
    show_store_prototype(curve_comp_on_txs)
    show_prototype(sensor_rx_assignment)
    show_prototype(sensor_tx_assignment)
    show_prototype(burst_count)
    show_prototype(disable)
    show_prototype(filter_bandwidth)
    show_prototype(stretch_duration)
    show_store_prototype(disable_noise_mitigation)
    show_store_prototype(freq_shift_noise_threshold)
    show_store_prototype(medium_noise_threshold)
    show_store_prototype(high_noise_threshold)
    show_store_prototype(noise_density)
    show_store_prototype(frame_count)
    show_store_prototype(iir_filter_coef)
    show_store_prototype(quiet_threshold)
    show_store_prototype(cmn_filter_disable)
    show_store_prototype(cmn_filter_max)
    show_store_prototype(touch_hysteresis)
    show_store_prototype(rx_low_edge_comp)
    show_store_prototype(rx_high_edge_comp)
    show_store_prototype(tx_low_edge_comp)
    show_store_prototype(tx_high_edge_comp)
    show_store_prototype(axis1_comp)
    show_store_prototype(axis2_comp)
    show_prototype(noise_control_1)
    show_prototype(noise_control_2)
    show_prototype(noise_control_3)
    show_store_prototype(no_signal_clarity)
    show_store_prototype(cbc_cap_0d)
    show_store_prototype(cbc_polarity_0d)
    show_store_prototype(cbc_tx_carrier_selection_0d)
/*Add synaptics capacitor test function */
#ifdef MMITEST
    show_prototype(mmi_test)
    show_prototype(mmi_test_result)
#endif
static ssize_t synaptics_rmi4_f54_data_read(struct file *data_file,
					    struct kobject *kobj,
					    struct bin_attribute *attributes,
					    char *buf, loff_t pos,
					    size_t count);

static struct attribute *attrs[] = {
	attrify(tp_status),
	attrify(report_size),
	attrify(no_auto_cal),
	attrify(report_type),
	attrify(fifoindex),
	attrify(do_preparation),
	attrify(get_report),
	attrify(force_cal),
	attrify(resume_touch),
	attrify(num_of_mapped_rx),
	attrify(num_of_mapped_tx),
	attrify(num_of_rx_electrodes),
	attrify(num_of_tx_electrodes),
	attrify(has_image16),
	attrify(has_image8),
	attrify(has_baseline),
	attrify(clock_rate),
	attrify(touch_controller_family),
	attrify(has_pixel_touch_threshold_adjustment),
	attrify(has_sensor_assignment),
	attrify(has_interference_metric),
	attrify(has_sense_frequency_control),
	attrify(has_firmware_noise_mitigation),
	attrify(has_two_byte_report_rate),
	attrify(has_one_byte_report_rate),
	attrify(has_relaxation_control),
	attrify(curve_compensation_mode),
	attrify(has_iir_filter),
	attrify(has_cmn_removal),
	attrify(has_cmn_maximum),
	attrify(has_touch_hysteresis),
	attrify(has_edge_compensation),
	attrify(has_per_frequency_noise_control),
	attrify(has_signal_clarity),
	attrify(number_of_sensing_frequencies),
#ifdef MMITEST
	attrify(mmi_test),
	attrify(mmi_test_result),
#endif
	NULL,
};

static struct attribute_group attr_group = GROUP(attrs);

static struct attribute *attrs_reg_0[] = {
	attrify(no_relax),
	attrify(no_scan),
	NULL,
};

static struct attribute *attrs_reg_1[] = {
	attrify(bursts_per_cluster),
	NULL,
};

static struct attribute *attrs_reg_2[] = {
	attrify(saturation_cap),
	NULL,
};

static struct attribute *attrs_reg_3[] = {
	attrify(pixel_touch_threshold),
	NULL,
};

static struct attribute *attrs_reg_4__6[] = {
	attrify(rx_feedback_cap),
	attrify(low_ref_cap),
	attrify(low_ref_feedback_cap),
	attrify(low_ref_polarity),
	attrify(high_ref_cap),
	attrify(high_ref_feedback_cap),
	attrify(high_ref_polarity),
	NULL,
};

static struct attribute *attrs_reg_7[] = {
	attrify(cbc_cap),
	attrify(cbc_polarity),
	attrify(cbc_tx_carrier_selection),
	NULL,
};

static struct attribute *attrs_reg_8__9[] = {
	attrify(integration_duration),
	attrify(reset_duration),
	NULL,
};

static struct attribute *attrs_reg_10[] = {
	attrify(noise_sensing_bursts_per_image),
	NULL,
};

static struct attribute *attrs_reg_11[] = {
	NULL,
};

static struct attribute *attrs_reg_12__13[] = {
	attrify(slow_relaxation_rate),
	attrify(fast_relaxation_rate),
	NULL,
};

static struct attribute *attrs_reg_14__16[] = {
	attrify(rxs_on_xaxis),
	attrify(curve_comp_on_txs),
	attrify(sensor_rx_assignment),
	attrify(sensor_tx_assignment),
	NULL,
};

static struct attribute *attrs_reg_17__19[] = {
	attrify(burst_count),
	attrify(disable),
	attrify(filter_bandwidth),
	attrify(stretch_duration),
	NULL,
};

static struct attribute *attrs_reg_20[] = {
	attrify(disable_noise_mitigation),
	NULL,
};

static struct attribute *attrs_reg_21[] = {
	attrify(freq_shift_noise_threshold),
	NULL,
};

static struct attribute *attrs_reg_22__26[] = {
	attrify(medium_noise_threshold),
	attrify(high_noise_threshold),
	attrify(noise_density),
	attrify(frame_count),
	NULL,
};

static struct attribute *attrs_reg_27[] = {
	attrify(iir_filter_coef),
	NULL,
};

static struct attribute *attrs_reg_28[] = {
	attrify(quiet_threshold),
	NULL,
};

static struct attribute *attrs_reg_29[] = {
	attrify(cmn_filter_disable),
	NULL,
};

static struct attribute *attrs_reg_30[] = {
	attrify(cmn_filter_max),
	NULL,
};

static struct attribute *attrs_reg_31[] = {
	attrify(touch_hysteresis),
	NULL,
};

static struct attribute *attrs_reg_32__35[] = {
	attrify(rx_low_edge_comp),
	attrify(rx_high_edge_comp),
	attrify(tx_low_edge_comp),
	attrify(tx_high_edge_comp),
	NULL,
};

static struct attribute *attrs_reg_36[] = {
	attrify(axis1_comp),
	NULL,
};

static struct attribute *attrs_reg_37[] = {
	attrify(axis2_comp),
	NULL,
};

static struct attribute *attrs_reg_38__40[] = {
	attrify(noise_control_1),
	attrify(noise_control_2),
	attrify(noise_control_3),
	NULL,
};

static struct attribute *attrs_reg_41[] = {
	attrify(no_signal_clarity),
	NULL,
};

static struct attribute *attrs_reg_57[] = {
	attrify(cbc_cap_0d),
	attrify(cbc_polarity_0d),
	attrify(cbc_tx_carrier_selection_0d),
	NULL,
};

static struct attribute_group attrs_ctrl_regs[] = {
	GROUP(attrs_reg_0),
	GROUP(attrs_reg_1),
	GROUP(attrs_reg_2),
	GROUP(attrs_reg_3),
	GROUP(attrs_reg_4__6),
	GROUP(attrs_reg_7),
	GROUP(attrs_reg_8__9),
	GROUP(attrs_reg_10),
	GROUP(attrs_reg_11),
	GROUP(attrs_reg_12__13),
	GROUP(attrs_reg_14__16),
	GROUP(attrs_reg_17__19),
	GROUP(attrs_reg_20),
	GROUP(attrs_reg_21),
	GROUP(attrs_reg_22__26),
	GROUP(attrs_reg_27),
	GROUP(attrs_reg_28),
	GROUP(attrs_reg_29),
	GROUP(attrs_reg_30),
	GROUP(attrs_reg_31),
	GROUP(attrs_reg_32__35),
	GROUP(attrs_reg_36),
	GROUP(attrs_reg_37),
	GROUP(attrs_reg_38__40),
	GROUP(attrs_reg_41),
	GROUP(attrs_reg_57),
};

static bool attrs_ctrl_regs_exist[ARRAY_SIZE(attrs_ctrl_regs)];

static struct bin_attribute dev_report_data = {
	.attr = {
		 .name = "report_data",
		 .mode = S_IRUGO,
		 },
	.size = 0,
	.read = synaptics_rmi4_f54_data_read,
};

static struct synaptics_rmi4_f54_handle *f54;
static struct synaptics_rmi4_f55_handle *f55;

DECLARE_COMPLETION(synptics_f54_s3207_remove_complete);

static bool is_report_type_valid(enum f54_report_types report_type)
{
	switch (report_type) {
	case F54_8BIT_IMAGE:
	case F54_16BIT_IMAGE:
	case F54_RAW_16BIT_IMAGE:
	case F54_HIGH_RESISTANCE:
	case F54_TX_TO_TX_SHORT:
	case F54_RX_TO_RX1:
	case F54_TRUE_BASELINE:
	case F54_FULL_RAW_CAP_MIN_MAX:
	case F54_RX_OPENS1:
	case F54_TX_OPEN:
	case F54_TX_TO_GROUND:
	case F54_RX_TO_RX2:
	case F54_RX_OPENS2:
	case F54_FULL_RAW_CAP:
	case F54_FULL_RAW_CAP_RX_COUPLING_COMP:
	case F54_SENSOR_SPEED:
	case F54_ADC_RANGE:
	case F54_TREX_OPENS:
	case F54_TREX_TO_GND:
	case F54_TREX_SHORTS:
		return true;
		break;
	default:
		f54->report_type = INVALID_REPORT_TYPE;
		f54->report_size = 0;
		return false;
	}
}

static void set_report_size(void)
{
	int retval = NO_ERR;
	struct synaptics_rmi4_data *rmi4_data = f54->rmi4_data;
	rx = f54->rx_assigned;
	tx = f54->tx_assigned;

	switch (f54->report_type) {
	case F54_8BIT_IMAGE:
		f54->report_size = rx * tx;
		break;
	case F54_16BIT_IMAGE:
	case F54_RAW_16BIT_IMAGE:
	case F54_TRUE_BASELINE:
	case F54_FULL_RAW_CAP:
	case F54_FULL_RAW_CAP_RX_COUPLING_COMP:
	case F54_SENSOR_SPEED:
		f54->report_size = 2 * rx * tx;
		break;
	case F54_HIGH_RESISTANCE:
		f54->report_size = HIGH_RESISTANCE_DATA_SIZE;
		break;
	case F54_TX_TO_TX_SHORT:
	case F54_TX_OPEN:
	case F54_TX_TO_GROUND:
		f54->report_size = 3;	/*(tx + 7) / 8; */
		break;
	case F54_RX_TO_RX1:
	case F54_RX_OPENS1:
		if (rx < tx)
			f54->report_size = 2 * rx * rx;
		else
			f54->report_size = 2 * rx * tx;
		break;
	case F54_FULL_RAW_CAP_MIN_MAX:
		f54->report_size = FULL_RAW_CAP_MIN_MAX_DATA_SIZE;
		break;
	case F54_RX_TO_RX2:
	case F54_RX_OPENS2:
		if (rx <= tx)
			f54->report_size = 0;
		else
			f54->report_size = 2 * rx * (rx - tx);
		break;
	case F54_ADC_RANGE:
		if (f54->query.has_signal_clarity) {
			mutex_lock(&f54->control_mutex);
			retval = f54->fn_ptr->read(rmi4_data,
						   f54->control.reg_41->address,
						   f54->control.reg_41->data,
						   sizeof(f54->control.reg_41->
							  data));
			mutex_unlock(&f54->control_mutex);
			if (retval < 0) {
				TS_LOG_ERR
				    ("%s: Failed to read control reg_41\n",
				     __func__);
				f54->report_size = 0;
				break;
			}
			if (!f54->control.reg_41->no_signal_clarity) {
				if (tx % 4)
					tx += 4 - (tx % 4);
			}
		}
		f54->report_size = 2 * rx * tx;
		break;
	case F54_TREX_OPENS:
	case F54_TREX_TO_GND:
	case F54_TREX_SHORTS:
		f54->report_size = TREX_DATA_SIZE;
		break;
	default:
		f54->report_size = 0;
	}

	return;
}

static int do_preparation(void)
{
	int retval = NO_ERR;
	unsigned char value = 0;
	unsigned char command = 0;
	unsigned char timeout_count = 0;
	struct synaptics_rmi4_data *rmi4_data = f54->rmi4_data;

	mutex_lock(&f54->control_mutex);

	if (f54->query.touch_controller_family == 1) {
		value = 0;
		retval = f54->fn_ptr->write(rmi4_data,
					    f54->control.reg_7->address,
					    &value,
					    sizeof(f54->control.reg_7->data));
		if (retval < 0) {
			TS_LOG_ERR("%s: Failed to disable CBC\n", __func__);
			mutex_unlock(&f54->control_mutex);
			return retval;
		}
	} else if (f54->query.has_ctrl88 == 1) {
		retval = f54->fn_ptr->read(rmi4_data,
					   f54->control.reg_88->address,
					   f54->control.reg_88->data,
					   sizeof(f54->control.reg_88->data));
		if (retval < 0) {
			TS_LOG_ERR("%s: Failed to disable CBC (read ctrl88)\n",
				   __func__);
			mutex_unlock(&f54->control_mutex);
			return retval;
		}
		f54->control.reg_88->cbc_polarity = 0;
		f54->control.reg_88->cbc_tx_carrier_selection = 0;
		retval = f54->fn_ptr->write(rmi4_data,
					    f54->control.reg_88->address,
					    f54->control.reg_88->data,
					    sizeof(f54->control.reg_88->data));
		if (retval < 0) {
			TS_LOG_ERR("%s: Failed to disable CBC (write ctrl88)\n",
				   __func__);
			mutex_unlock(&f54->control_mutex);
			return retval;
		}
	}

	if (f54->query.has_0d_acquisition_control) {
		value = 0;
		retval = f54->fn_ptr->write(rmi4_data,
					    f54->control.reg_57->address,
					    &value,
					    sizeof(f54->control.reg_57->data));
		if (retval < 0) {
			TS_LOG_ERR("%s: Failed to disable 0D CBC\n", __func__);
			mutex_unlock(&f54->control_mutex);
			return retval;
		}
	}

	if (f54->query.has_signal_clarity) {
		value = 1;
		retval = f54->fn_ptr->write(rmi4_data,
					    f54->control.reg_41->address,
					    &value,
					    sizeof(f54->control.reg_41->data));
		if (retval < 0) {
			TS_LOG_ERR("%s: Failed to disable signal clarity\n",
				   __func__);
			mutex_unlock(&f54->control_mutex);
			return retval;
		}
	}

	mutex_unlock(&f54->control_mutex);

	command = (unsigned char)COMMAND_FORCE_UPDATE;

	retval = f54->fn_ptr->write(rmi4_data,
				    f54->command_base_addr,
				    &command, sizeof(command));
	if (retval < 0) {
		TS_LOG_ERR("%s: Failed to write force update command\n",
			   __func__);
		return retval;
	}

	timeout_count = 0;
	do {
		retval = f54->fn_ptr->read(rmi4_data,
					   f54->command_base_addr,
					   &value, sizeof(value));
		if (retval < 0) {
			TS_LOG_ERR("%s: Failed to read command register\n",
				   __func__);
			return retval;
		}

		if (value == 0x00)
			break;

		msleep(100);
		timeout_count++;
	} while (timeout_count < FORCE_TIMEOUT_100MS);

	if (timeout_count == FORCE_TIMEOUT_100MS) {
		TS_LOG_ERR("%s: Timed out waiting for force update\n",
			   __func__);
		return -ETIMEDOUT;
	}

	command = (unsigned char)COMMAND_FORCE_CAL;

	retval = f54->fn_ptr->write(rmi4_data,
				    f54->command_base_addr,
				    &command, sizeof(command));
	if (retval < 0) {
		TS_LOG_ERR("%s: Failed to write force cal command\n", __func__);
		return retval;
	}

	timeout_count = 0;
	do {
		retval = f54->fn_ptr->read(rmi4_data,
					   f54->command_base_addr,
					   &value, sizeof(value));
		if (retval < 0) {
			TS_LOG_ERR("%s: Failed to read command register\n",
				   __func__);
			return retval;
		}

		if (value == 0x00)
			break;

		msleep(100);
		timeout_count++;
	} while (timeout_count < FORCE_TIMEOUT_100MS);

	if (timeout_count == FORCE_TIMEOUT_100MS) {
		TS_LOG_ERR("%s: Timed out waiting for force cal\n", __func__);
		return -ETIMEDOUT;
	}

	return 0;
}

#ifdef WATCHDOG_HRTIMER
static void timeout_set_status(struct work_struct *work)
{
	int retval = NO_ERR;
	unsigned char command = 0;
	struct synaptics_rmi4_data *rmi4_data = f54->rmi4_data;

	TS_LOG_INFO("watchdog timeout f54->status is %d\n", f54->status);

	mutex_lock(&f54->status_mutex);
	if (f54->status == STATUS_BUSY) {
		retval = f54->fn_ptr->read(rmi4_data,
					   f54->command_base_addr,
					   &command, sizeof(command));
		if (retval < 0) {
			TS_LOG_ERR("%s: Failed to read command register\n",
				   __func__);
		} else if (command & COMMAND_GET_REPORT) {
			TS_LOG_ERR("%s: Report type not supported by FW\n",
				   __func__);
		} else {
			queue_delayed_work(f54->status_workqueue,
					   &f54->status_work, 0);
			mutex_unlock(&f54->status_mutex);
			return;
		}
		f54->report_type = INVALID_REPORT_TYPE;
		f54->report_size = 0;
		f54->status = STATUS_IDLE;
	}
	mutex_unlock(&f54->status_mutex);

	return;
}

static enum hrtimer_restart get_report_timeout(struct hrtimer *timer)
{
	TS_LOG_DEBUG("%s:in!\n", __func__);
	schedule_work(&(f54->timeout_work));

	return HRTIMER_NORESTART;
}
#endif

#ifdef RAW_HEX
static void print_raw_hex_report(void)
{
	unsigned int ii = 0;

	pr_info("%s: Report data (raw hex)\n", __func__);

	switch (f54->report_type) {
	case F54_16BIT_IMAGE:
	case F54_RAW_16BIT_IMAGE:
	case F54_HIGH_RESISTANCE:
	case F54_TRUE_BASELINE:
	case F54_FULL_RAW_CAP_MIN_MAX:
	case F54_FULL_RAW_CAP:
	case F54_FULL_RAW_CAP_RX_COUPLING_COMP:
	case F54_SENSOR_SPEED:
	case F54_ADC_RANGE:
		for (ii = 0; ii < f54->report_size; ii += 2) {
			pr_info("%03d: 0x%02x%02x\n",
				ii / 2,
				f54->report_data[ii + 1], f54->report_data[ii]);
		}
		break;
	default:
		for (ii = 0; ii < f54->report_size; ii++)
			pr_info("%03d: 0x%02x\n", ii, f54->report_data[ii]);
		break;
	}

	return;
}
#endif

#ifdef HUMAN_READABLE
static void print_image_report(void)
{
	unsigned int ii = 0;
	unsigned int jj = 0;
	short *report_data = NULL;

	switch (f54->report_type) {
	case F54_16BIT_IMAGE:
	case F54_RAW_16BIT_IMAGE:
	case F54_TRUE_BASELINE:
	case F54_FULL_RAW_CAP:
	case F54_FULL_RAW_CAP_RX_COUPLING_COMP:
		pr_info("%s: Report data (image)\n", __func__);

		report_data = (short *)f54->report_data;

		for (ii = 0; ii < f54->tx_assigned; ii++) {
			for (jj = 0; jj < f54->rx_assigned; jj++) {
				if (*report_data < -64)
					pr_cont(".");
				else if (*report_data < 0)
					pr_cont("-");
				else if (*report_data > 64)
					pr_cont("*");
				else if (*report_data > 0)
					pr_cont("+");
				else
					pr_cont("0");

				report_data++;
			}
			pr_info("");
		}
		pr_info("%s: End of report\n", __func__);
		break;
	default:
		pr_info("%s: Image not supported for report type %d\n",
			__func__, f54->report_type);
	}

	return;
}
#endif

static void free_control_mem(void)
{
	struct f54_control control = f54->control;

	kfree(control.reg_0);
	kfree(control.reg_1);
	kfree(control.reg_2);
	kfree(control.reg_3);
	kfree(control.reg_4__6);
	kfree(control.reg_7);
	kfree(control.reg_8__9);
	kfree(control.reg_10);
	kfree(control.reg_11);
	kfree(control.reg_12__13);
	kfree(control.reg_14);
	kfree(control.reg_15);
	kfree(control.reg_16);
	kfree(control.reg_17);
	kfree(control.reg_18);
	kfree(control.reg_19);
	kfree(control.reg_20);
	kfree(control.reg_21);
	kfree(control.reg_22__26);
	kfree(control.reg_27);
	kfree(control.reg_28);
	kfree(control.reg_29);
	kfree(control.reg_30);
	kfree(control.reg_31);
	kfree(control.reg_32__35);
	kfree(control.reg_36);
	kfree(control.reg_37);
	kfree(control.reg_38);
	kfree(control.reg_39);
	kfree(control.reg_40);
	kfree(control.reg_41);
	kfree(control.reg_57);

	return;
}

/*
static void remove_sysfs(void)
{
	int reg_num;

	sysfs_remove_bin_file(f54->attr_dir, &dev_report_data);

	sysfs_remove_group(f54->attr_dir, &attr_group);

	for (reg_num = 0; reg_num < ARRAY_SIZE(attrs_ctrl_regs); reg_num++)
		sysfs_remove_group(f54->attr_dir, &attrs_ctrl_regs[reg_num]);

	kobject_put(f54->attr_dir);

	return;
}
*/
static ssize_t synaptics_rmi4_f54_tp_status_show(struct device *dev,
						 struct device_attribute *attr,
						 char *buf)
{
	return snprintf(buf, PAGE_SIZE, "%u\n", f54->status);
}

static ssize_t synaptics_rmi4_f54_report_size_show(struct device *dev,
						   struct device_attribute
						   *attr, char *buf)
{
	return snprintf(buf, PAGE_SIZE, "%u\n", f54->report_size);
}

static ssize_t synaptics_rmi4_f54_no_auto_cal_show(struct device *dev,
						   struct device_attribute
						   *attr, char *buf)
{
	return snprintf(buf, PAGE_SIZE, "%u\n", f54->no_auto_cal);
}

static ssize_t synaptics_rmi4_f54_no_auto_cal_store(struct device *dev,
						    struct device_attribute
						    *attr, const char *buf,
						    size_t count)
{
	int retval = NO_ERR;
	unsigned char data = 0;
	unsigned long setting = 0;
	struct synaptics_rmi4_data *rmi4_data = f54->rmi4_data;

	retval = sstrtoul(buf, 10, &setting);
	if (retval)
		return retval;

	if (setting > 1)
		return -EINVAL;

	retval = f54->fn_ptr->read(rmi4_data,
				   f54->control_base_addr, &data, sizeof(data));
	if (retval < 0) {
		TS_LOG_ERR("%s: Failed to read control register\n", __func__);
		return retval;
	}

	if ((data & NO_AUTO_CAL_MASK) == setting)
		return count;

	data = (data & ~NO_AUTO_CAL_MASK) | (data & NO_AUTO_CAL_MASK);

	retval = f54->fn_ptr->write(rmi4_data,
				    f54->control_base_addr,
				    &data, sizeof(data));
	if (retval < 0) {
		TS_LOG_ERR("%s: Failed to write control register\n", __func__);
		return retval;
	}

	f54->no_auto_cal = (setting == 1);

	return count;
}

static ssize_t synaptics_rmi4_f54_report_type_show(struct device *dev,
						   struct device_attribute
						   *attr, char *buf)
{
	return snprintf(buf, PAGE_SIZE, "%u\n", f54->report_type);
}

/*Add synaptics capacitor test function */
#ifdef MMITEST
static int synaptics_rmi4_f54_report_type_set(char setting)
{
	int retval = 0;
	unsigned char data = 0;
	struct synaptics_rmi4_data *rmi4_data = f54->rmi4_data;

	if (!is_report_type_valid((enum f54_report_types)setting)) {
		TS_LOG_ERR("%s: Report type not supported by driver\n",
			   __func__);
		return -EINVAL;
	}

	mutex_lock(&f54->status_mutex);

	if (f54->status != STATUS_BUSY) {
		f54->report_type = (enum f54_report_types)setting;
		data = (unsigned char)setting;
		retval = f54->fn_ptr->write(rmi4_data,
					    f54->data_base_addr,
					    &data, sizeof(data));
		mutex_unlock(&f54->status_mutex);
		if (retval < 0) {
			TS_LOG_ERR("%s: Failed to write data register\n",
				   __func__);
			return retval;
		}
		return 0;
	} else {
		TS_LOG_ERR("%s: Previous get report still ongoing\n", __func__);
		mutex_unlock(&f54->status_mutex);
		return -EINVAL;
	}
}
#endif

static ssize_t synaptics_rmi4_f54_report_type_store(struct device *dev,
						    struct device_attribute
						    *attr, const char *buf,
						    size_t count)
{
	int retval = NO_ERR;
	unsigned char data = 0;
	unsigned long setting = 0;
	struct synaptics_rmi4_data *rmi4_data = f54->rmi4_data;

	retval = sstrtoul(buf, 10, &setting);
	if (retval)
		return retval;

	if (!is_report_type_valid((enum f54_report_types)setting)) {
		TS_LOG_ERR("%s: Report type not supported by driver\n",
			   __func__);
		return -EINVAL;
	}

	mutex_lock(&f54->status_mutex);

	if (f54->status != STATUS_BUSY) {
		f54->report_type = (enum f54_report_types)setting;
		data = (unsigned char)setting;
		retval = f54->fn_ptr->write(rmi4_data,
					    f54->data_base_addr,
					    &data, sizeof(data));
		mutex_unlock(&f54->status_mutex);
		if (retval < 0) {
			TS_LOG_ERR("%s: Failed to write data register\n",
				   __func__);
			return retval;
		}
		return count;
	} else {
		TS_LOG_ERR("%s: Previous get report still ongoing\n", __func__);
		mutex_unlock(&f54->status_mutex);
		return -EINVAL;
	}
}

static ssize_t synaptics_rmi4_f54_fifoindex_show(struct device *dev,
						 struct device_attribute *attr,
						 char *buf)
{
	int retval = NO_ERR;
	unsigned char data[2] = { 0 };
	struct synaptics_rmi4_data *rmi4_data = f54->rmi4_data;

	retval = f54->fn_ptr->read(rmi4_data,
				   f54->data_base_addr +
				   DATA_REPORT_INDEX_OFFSET, data,
				   sizeof(data));
	if (retval < 0) {
		TS_LOG_ERR("%s: Failed to read data registers\n", __func__);
		return retval;
	}

	batohs(&f54->fifoindex, data);

	return snprintf(buf, PAGE_SIZE, "%u\n", f54->fifoindex);
}

static ssize_t synaptics_rmi4_f54_fifoindex_store(struct device *dev,
						  struct device_attribute *attr,
						  const char *buf, size_t count)
{
	int retval = NO_ERR;
	unsigned char data[2] = { 0 };
	unsigned long setting = 0;
	struct synaptics_rmi4_data *rmi4_data = f54->rmi4_data;

	retval = sstrtoul(buf, 10, &setting);
	if (retval)
		return retval;

	f54->fifoindex = setting;

	hstoba(data, (unsigned short)setting);

	retval = f54->fn_ptr->write(rmi4_data,
				    f54->data_base_addr +
				    DATA_REPORT_INDEX_OFFSET, data,
				    sizeof(data));
	if (retval < 0) {
		TS_LOG_ERR("%s: Failed to write data registers\n", __func__);
		return retval;
	}

	return count;
}

/*Add synaptics capacitor test function */
#ifdef MMITEST
static int synaptics_rmi4_f54_do_preparation_set(void)
{
	int retval = NO_ERR;
	/*struct synaptics_rmi4_data *rmi4_data = f54->rmi4_data; */

	mutex_lock(&f54->status_mutex);

	if (f54->status != STATUS_IDLE) {
		if (f54->status != STATUS_BUSY) {
			TS_LOG_ERR("%s: Invalid status (%d)\n", __func__,
				   f54->status);
		} else {
			TS_LOG_ERR("%s: Previous get report still ongoing\n",
				   __func__);
		}
		mutex_unlock(&f54->status_mutex);
		return -EBUSY;
	}

	mutex_unlock(&f54->status_mutex);

	retval = do_preparation();
	if (retval < 0) {
		TS_LOG_ERR("%s: Failed to do preparation\n", __func__);
		return retval;
	}
	return 0;
}
#endif

static ssize_t synaptics_rmi4_f54_do_preparation_store(struct device *dev,
						       struct device_attribute
						       *attr, const char *buf,
						       size_t count)
{
	int retval = NO_ERR;
	unsigned long setting = 0;
	/*/struct synaptics_rmi4_data *rmi4_data = f54->rmi4_data; */

	retval = sstrtoul(buf, 10, &setting);
	if (retval)
		return retval;

	if (setting != 1)
		return -EINVAL;

	mutex_lock(&f54->status_mutex);

	if (f54->status != STATUS_IDLE) {
		if (f54->status != STATUS_BUSY) {
			TS_LOG_ERR("%s: Invalid status (%d)\n", __func__,
				   f54->status);
		} else {
			TS_LOG_ERR("%s: Previous get report still ongoing\n",
				   __func__);
		}
		mutex_unlock(&f54->status_mutex);
		return -EBUSY;
	}

	mutex_unlock(&f54->status_mutex);

	retval = do_preparation();
	if (retval < 0) {
		TS_LOG_ERR("%s: Failed to do preparation\n", __func__);
		return retval;
	}

	return count;
}

/*Add synaptics capacitor test function */
#ifdef MMITEST
static ssize_t synaptics_rmi4_f54_get_report_set(char setting)
{
	int retval = NO_ERR;
	unsigned char command = 0;
	struct synaptics_rmi4_data *rmi4_data = f54->rmi4_data;

	if (setting != 1)
		return -EINVAL;

	command = (unsigned char)COMMAND_GET_REPORT;

	if (!is_report_type_valid(f54->report_type)) {
		TS_LOG_ERR("%s: Invalid report type\n", __func__);
		return -EINVAL;
	}

	mutex_lock(&f54->status_mutex);

	if (f54->status != STATUS_IDLE) {
		if (f54->status != STATUS_BUSY) {
			TS_LOG_ERR("%s: Invalid status (%d)\n", __func__,
				   f54->status);
		} else {
			TS_LOG_ERR("%s: Previous get report still ongoing\n",
				   __func__);
		}
		mutex_unlock(&f54->status_mutex);
		return -EBUSY;
	}
	/*TS_LOG_ERR("to set interrupt\n");*/
	/*set_interrupt(true);*/

	f54->status = STATUS_BUSY;

	retval = f54->fn_ptr->write(rmi4_data,
				    f54->command_base_addr,
				    &command, sizeof(command));
	mutex_unlock(&f54->status_mutex);
	if (retval < 0) {
		TS_LOG_ERR("%s: Failed to write get report command\n",
			   __func__);
		return retval;
	}
#ifdef WATCHDOG_HRTIMER
	hrtimer_start(&f54->watchdog,
		      ktime_set(WATCHDOG_TIMEOUT_S, 0), HRTIMER_MODE_REL);
#endif

	return 0;
}
#endif

static ssize_t synaptics_rmi4_f54_get_report_store(struct device *dev,
						   struct device_attribute
						   *attr, const char *buf,
						   size_t count)
{
	int retval = NO_ERR;
	unsigned char command = 0;
	unsigned long setting = 0;
	struct synaptics_rmi4_data *rmi4_data = f54->rmi4_data;

	retval = sstrtoul(buf, 10, &setting);
	if (retval)
		return retval;

	if (setting != 1)
		return -EINVAL;

	command = (unsigned char)COMMAND_GET_REPORT;

	if (!is_report_type_valid(f54->report_type)) {
		TS_LOG_ERR("%s: Invalid report type\n", __func__);
		return -EINVAL;
	}

	mutex_lock(&f54->status_mutex);

	if (f54->status != STATUS_IDLE) {
		if (f54->status != STATUS_BUSY) {
			TS_LOG_ERR("%s: Invalid status (%d)\n", __func__,
				   f54->status);
		} else {
			TS_LOG_ERR("%s: Previous get report still ongoing\n",
				   __func__);
		}
		mutex_unlock(&f54->status_mutex);
		return -EBUSY;
	}
	/*set_interrupt(true);*/

	f54->status = STATUS_BUSY;

	retval = f54->fn_ptr->write(rmi4_data,
				    f54->command_base_addr,
				    &command, sizeof(command));
	mutex_unlock(&f54->status_mutex);
	if (retval < 0) {
		TS_LOG_ERR("%s: Failed to write get report command\n",
			   __func__);
		return retval;
	}
#ifdef WATCHDOG_HRTIMER
	hrtimer_start(&f54->watchdog,
		      ktime_set(WATCHDOG_TIMEOUT_S, 0), HRTIMER_MODE_REL);
#endif

	return count;
}

static ssize_t synaptics_rmi4_f54_force_cal_store(struct device *dev,
						  struct device_attribute *attr,
						  const char *buf, size_t count)
{
	int retval = NO_ERR;
	unsigned char command = 0;
	unsigned long setting = 0;
	struct synaptics_rmi4_data *rmi4_data = f54->rmi4_data;

	retval = sstrtoul(buf, 10, &setting);
	if (retval)
		return retval;

	if (setting != 1)
		return -EINVAL;

	command = (unsigned char)COMMAND_FORCE_CAL;

	if (f54->status == STATUS_BUSY)
		return -EBUSY;

	retval = f54->fn_ptr->write(rmi4_data,
				    f54->command_base_addr,
				    &command, sizeof(command));
	if (retval < 0) {
		TS_LOG_ERR("%s: Failed to write force cal command\n", __func__);
		return retval;
	}

	return count;
}

/*Add synaptics capacitor test function */
#ifdef MMITEST
static struct kobject *touch_screen_kobject_ts;

static struct kobject *tp_get_touch_screen_obj(void)
{
	if (NULL == touch_screen_kobject_ts) {
		touch_screen_kobject_ts =
		    kobject_create_and_add("touch_screen", NULL);
		if (!touch_screen_kobject_ts) {
			TS_LOG_ERR("create touch_screen kobjetct error!\n");
			return NULL;
		} else {
			TS_LOG_INFO(" create sys/touch_screen successful!\n");
		}
	} else {
		TS_LOG_INFO("sys/touch_screen already exist!\n");
	}

	return touch_screen_kobject_ts;
}

static void mmi_rawimage_report(unsigned char *buffer)
{
	int i = 0, j = 0, k = 0;
	size_t len = 0; 
	short *DataArray = NULL;
	short temp = 0;
	enum mmi_results TestResult = TEST_PASS;
	char buf[DATA_TEMP_BUFF_SIZE] = { 0 };
	memset(g_mmi_buf_f54raw_data, 0,
	       (rx * tx * DATA_TEMP_BUFF_SIZE + F54_MAX_CAP_TITLE_SIZE));
	sprintf(g_mmi_buf_f54raw_data, "RawImageData:\n");
	len = rx * tx * DATA_TEMP_BUFF_SIZE + F54_MAX_CAP_TITLE_SIZE - strlen(g_mmi_buf_f54raw_data) - 1;
	DataArray = (short *)kmalloc(sizeof(short) * tx * rx, GFP_KERNEL);
	if (DataArray == NULL) {
		TS_LOG_ERR("%s:kmalloc failed\n",__func__);
		return;
	}

	for (i = 0; i < tx; i++) {
		for (j = 0; j < rx; j++) {
			temp = buffer[k] | (buffer[k + 1] << 8);
			DataArray[i * rx + j] = temp;	/*float is not allowed in kernel space*/
			k = k + 2;

			snprintf(buf, sizeof(buf),"%d ", temp);
			strncat(g_mmi_buf_f54raw_data, buf, len);
		}
		strncat(g_mmi_buf_f54raw_data, "\n", 1);
	}

	for (i = 0; i < tx; i++) {
		for (j = 0; j < rx; j++) {
			if ((DataArray[i * rx + j] >
			     FullRawCapUpperLimit[i * rx + j])
			    || (DataArray[i * rx + j] <
				FullRawCapLowerLimit[i * rx + j])) {
				TestResult = TEST_FAILED;
				TS_LOG_ERR
				    ("%s: TEST_FAILED:1F-, RawCap(%d,%d) = %d \n",
				     __func__, i, j, DataArray[i * rx + j]);
			}
		}
	}

	if (TestResult) {
		strcat(g_mmi_buf_f54test_result, "1P-");
	} else {
		strcat(g_mmi_buf_f54test_result, "1F-");
		strncpy(tp_test_failed_reason, "panel_reason",
			TP_TEST_FAILED_REASON_LEN);
	}

	kfree(DataArray);
	return;
}

static int RxtoRx1ShortTest(unsigned char *buffer)
{

	int i = 0, j = 0, k = 0;
	int count = 0;
	size_t len = 0;
	int DiagonalUpperLimit = RxDiagonalUpperLimit;
	int DiagonalLowerLimit = RxDiagonalLowerLimit;
	int OthersUpperLimit = RxOthersUpperLimit;
	short ImageArray = 0;
	char buf[DATA_TEMP_BUFF_SIZE] = { 0 };
	

	for (i = 0; i < tx; i++) {
		for (j = 0; j < rx; j++) {
			ImageArray = buffer[k] | (buffer[k + 1] << 8);
			if(ImageArray< 0){
				continue;
			}
			k = k + 2;
			snprintf(buf, sizeof(buf),"%5d ", ImageArray);
			if (i == j) {
				len = 6 * rx + F54_MAX_CAP_TITLE_SIZE - strlen(g_mmi_RxtoRxshort_report) - 1;
				strncat(g_mmi_RxtoRxshort_report, buf,
					len);
				if ((ImageArray <= DiagonalUpperLimit)
				    && (ImageArray >= DiagonalLowerLimit))
					count++;
			} else {
				if (ImageArray <= OthersUpperLimit)
					count++;
			}
		}
		/*strncat(g_mmi_RxtoRxshort_report, "\n", 1);*/
	}
	return count;
}

static int RxtoRx2ShortTest(unsigned char *buffer)
{
	int i = 0, j = 0, k = 0;
	int count = 0;
	size_t len = 0;
	int DiagonalUpperLimit = RxDiagonalUpperLimit;
	int DiagonalLowerLimit = RxDiagonalLowerLimit;
	int OthersUpperLimit = RxOthersUpperLimit;
	short ImageArray = 0;
	char buf[DATA_TEMP_BUFF_SIZE] = { 0 };

	for (i = 0; i < (rx - tx); i++) {
		for (j = 0; j < rx; j++) {
			ImageArray = buffer[k] | (buffer[k + 1] << 8);
			if(ImageArray< 0){
				continue;
			}
			k = k + 2;
			snprintf(buf, sizeof(buf),"%5d ", ImageArray);

			if ((i + tx) == j) {
				len = 6 * rx + F54_MAX_CAP_TITLE_SIZE - strlen(g_mmi_RxtoRxshort_report) - 1;
				strncat(g_mmi_RxtoRxshort_report, buf,
					len);
				if ((ImageArray <= DiagonalUpperLimit)
				    && (ImageArray >= DiagonalLowerLimit))
					count++;
				else
					TS_LOG_ERR
					    ("%s: Failed,ImageArray(%d,%d) = %d\n",
					     __func__, i, j, ImageArray);
			} else {
				if (ImageArray <= OthersUpperLimit)
					count++;
				else
					TS_LOG_ERR
					    ("%s: Failed,ImageArray(%d,%d) = %d\n",
					     __func__, i, j, ImageArray);
			}
		}
		/*strncat(g_mmi_RxtoRxshort_report, "\n", 1);*/
	}
	strncat(g_mmi_RxtoRxshort_report, "\n", 1);
	return count;
}

static void mmi_RxtoRxshort1_report(unsigned char *buffer)
{

	int count = 0;
	/*enum mmi_results TestResult = TEST_PASS;*/
	memset(g_mmi_RxtoRxshort_report, 0, (6 * rx + F54_MAX_CAP_TITLE_SIZE));	/*only Diagonal for V3*/
	sprintf(g_mmi_RxtoRxshort_report, "RxtoRxshort:\n");

	count = RxtoRx1ShortTest(buffer);
	/*count += RxtoRx2ShortTest(buffer);*/

	if (count == (rx * rx)) {
		/*TestResult = TEST_PASS;*/
		strcat(g_mmi_buf_f54test_result, "4P-");
	} else {
		/*TestResult = TEST_FAILED;*/
		/*strcat(g_mmi_buf_f54test_result,"4F-");*/
		TS_LOG_ERR("%s:test failed,count = %d LINE = %d\n", __func__,
			   count, __LINE__);
	}

	return;
}

static void mmi_RxtoRxshort2_report(unsigned char *buffer)
{

	int count = tx * rx;
	/*enum mmi_results TestResult = TEST_PASS;*/

	count += RxtoRx2ShortTest(buffer);

	if (count == (rx * rx)) {
		/*TestResult = TEST_PASS;*/
		strcat(g_mmi_buf_f54test_result, "4P-");
	} else {
		/*TestResult = TEST_FAILED;*/
		strcat(g_mmi_buf_f54test_result, "4F-");
		strncpy(tp_test_failed_reason, "panel_reason",
			TP_TEST_FAILED_REASON_LEN);
	}

	return;
}

static void mmi_txtotx_short_report(unsigned char *buffer)
{
	int i = 0, j = 0, index = 0;
	int numberOfBytes = (f54->tx_assigned + 7) / 8;
	char val = 0;
	enum mmi_results TestResult = TEST_PASS;

	for (i = 0; i < numberOfBytes; i++) {
		for (j = 0; j < 8; j++) {
			index = i * 8 + j;
			if (index >= f54->tx_assigned)
				break;
			val = (buffer[i] & (1 << j)) >> j;
			if (numberOfBytes < tx) {
				if (val != TxTxReportLimit) {
					TestResult = TEST_FAILED;
					TS_LOG_ERR("%s: Failed,val(%d,%d) = %d",
						   __func__, i, j, val);
				}
			}
		}
	}

	if (TestResult) {
		strcat(g_mmi_buf_f54test_result, "2P-");
	} else {
		strcat(g_mmi_buf_f54test_result, "2F-");
		strncpy(tp_test_failed_reason, "panel_reason",
			TP_TEST_FAILED_REASON_LEN);
	}
	return;
}

static void mmi_txtoground_short_report(unsigned char *buffer,
					size_t report_size)
{
	/*enum mmi_results TestResult = TEST_PASS;*/

	char Txstatus = 0;
	int result = 0;
	int i = 0, j = 0;

	for (i = 0; i < report_size; i++) {
		for (j = 0; j < 8; j++) {
			Txstatus = (buffer[i] & (1 << j)) >> j;
			if (1 == Txstatus)
				result++;
		}
	}

	if ((tx) == result) {
		/*TestResult = TEST_PASS;*/
		strcat(g_mmi_buf_f54test_result, "3P-");
	} else {
		TS_LOG_ERR("%s: Failed in txtoground, result = %d\n", __func__,
			   result);
		/*TestResult = TEST_FAILED;*/
		strcat(g_mmi_buf_f54test_result, "3F-");
		strncpy(tp_test_failed_reason, "panel_reason",
			TP_TEST_FAILED_REASON_LEN);
	}

	return;
}

static void mmi_maxmincapacitance_report(unsigned char *buffer)
{
	/*enum mmi_results TestResult = TEST_PASS;*/

	short maxcapacitance = FullRawMaxCap;
	short mincapacitance = FullRawMinCap;
	short max = 0;
	short min = 0;
	size_t len = 0;
	char buf[2 * DATA_TEMP_BUFF_SIZE] = { 0 };
	memset(g_mmi_maxmincapacitance_report, 0,
	       (2 * DATA_TEMP_BUFF_SIZE + F54_MAX_CAP_TITLE_SIZE));
	sprintf(g_mmi_maxmincapacitance_report, "maxmincapacitance:\n");
	max = (buffer[0]) | (buffer[1] << 8);
	min = (buffer[2]) | (buffer[3] << 8);
	len = 2 * DATA_TEMP_BUFF_SIZE + F54_MAX_CAP_TITLE_SIZE - strlen(g_mmi_maxmincapacitance_report) - 1;
	if ((max < maxcapacitance) && (min > mincapacitance)) {
		/*TestResult = TEST_PASS;*/
		strcat(g_mmi_buf_f54test_result, "5P-");
	} else {
		/*TestResult = TEST_FAILED;*/
		strcat(g_mmi_buf_f54test_result, "5F-");
		strncpy(tp_test_failed_reason, "panel_reason",
			TP_TEST_FAILED_REASON_LEN);
	}

	snprintf(buf, sizeof(buf)," %d %d", max, min);
	strncat(g_mmi_maxmincapacitance_report, buf, len);
	strncat(g_mmi_maxmincapacitance_report, "\n", 1);

	return;
}

static void mmi_highresistance_report(unsigned char *buffer)
{
	int i = 0, k = 0;
	short temp = 0;
	size_t len = 0;
	enum mmi_results TestResult = TEST_PASS;
	short HighResistanceResult[3] = { 0 };
	char buf[DATA_TEMP_BUFF_SIZE] = { 0 };
	memset(g_mmi_highresistance_report, 0,
	       (3 * DATA_TEMP_BUFF_SIZE + F54_MAX_CAP_TITLE_SIZE));
	sprintf(g_mmi_highresistance_report, "highresistance:\n");
	len = 3 * DATA_TEMP_BUFF_SIZE + F54_MAX_CAP_TITLE_SIZE -strlen(g_mmi_highresistance_report)-1;
	for (i = 0; i < 3; i++, k += 2) {
		temp = buffer[k] | (buffer[k + 1] << 8);
		HighResistanceResult[i] = temp;
		if ((HighResistanceResult[i] > HighResistanceUpperLimit[i]) ||
		    (HighResistanceResult[i] < HighResistanceLowerLimit[i])) {

			TestResult = TEST_FAILED;
			TS_LOG_ERR
			    ("%s: TEST_FAILED: 6F ,highresistance[%d] = %d \n",
			     __func__, i, HighResistanceResult[i]);
		}
		snprintf(buf, sizeof(buf), " %d ", HighResistanceResult[i]);
		strncat(g_mmi_highresistance_report, buf, len);
	}

	strncat(g_mmi_highresistance_report, "\n", 1);

	if (TestResult) {
		strcat(g_mmi_buf_f54test_result, "6P-");
	} else {
		strcat(g_mmi_buf_f54test_result, "6F-");
		strncpy(tp_test_failed_reason, "panel_reason",
			TP_TEST_FAILED_REASON_LEN);
	}
	return;
}

static void mmi_delta_report(unsigned char *buffer)
{
	int i = 0, j = 0, k = 0;
	short *DataArray = NULL;
	short temp = 0;
	char buf[DATA_TEMP_BUFF_SIZE] = { 0 };
	size_t len = 0;
	TS_LOG_INFO("mmi_delta_report\n");
	memset(g_buf_debug_data, 0,
	       (rx * tx * DATA_TEMP_BUFF_SIZE + F54_MAX_CAP_TITLE_SIZE + F54_MAX_CAP_TITLE_SIZE +
		F54_MAX_CAP_TITLE_SIZE));
	sprintf(g_buf_debug_data, "delta_data:\n");
	len = rx * tx * DATA_TEMP_BUFF_SIZE + F54_MAX_CAP_TITLE_SIZE + F54_MAX_CAP_TITLE_SIZE +
		F54_MAX_CAP_TITLE_SIZE - strlen(g_buf_debug_data) -1;
	DataArray = (short *)kmalloc(sizeof(short) * tx * rx, GFP_KERNEL);
	if (DataArray == NULL) {
		TS_LOG_ERR("%s:kmalloc failed\n",__func__);
		return;
	}
	for (i = 0; i < tx; i++) {
		for (j = 0; j < rx; j++) {
			temp = buffer[k] | (buffer[k + 1] << 8);
			DataArray[i * rx + j] = temp;	/*float is not allowed in kernel space*/
			k = k + 2;

			snprintf(buf, sizeof(buf),"%d ", temp);
			strncat(g_buf_debug_data, buf, len);
		}
		strcat(g_buf_debug_data, "\n");
	}

	kfree(DataArray);
	return;
}

static int mmi_runtest(int report_type)
{
	int retval = 0;
	unsigned char patience = 10;
	int report_size = 0;
	unsigned char *buffer = NULL;
	unsigned char command = 0;
	/*struct synaptics_rmi4_data *rmi4_data = f54->rmi4_data;*/

	/*TS_LOG_INFO("mmi test report type is %d",report_type);*/

	retval = synaptics_rmi4_f54_report_type_set(report_type);
	if (retval)
		goto test_exit;
	mdelay(5);
	retval = synaptics_rmi4_f54_get_report_set(1);
	if (retval) {
		TS_LOG_ERR
		    ("synaptics_rmi4_f54_get_report_set failed  retval=%d",
		     retval);
		goto test_exit;
	}
	while (patience != 0) {
		msleep(150);
		retval = f54->fn_ptr->read(f54->rmi4_data,
					   f54->command_base_addr,
					   &command, sizeof(command));
		if (retval) {
			TS_LOG_ERR("read command failed  retval=%d", retval);
			continue;
		}
		TS_LOG_DEBUG("command=%d", command);
		if (0 == (command & 0x01)) {
			break;
		}
		patience--;
	}
	retval = synaptics_rmi4_f54_attention();
	if (retval) {
		TS_LOG_ERR("synaptics_rmi4_f54_attention failed  retval=%d",
			   retval);
		goto test_exit;
	}

	report_size = f54->report_size;

	TS_LOG_INFO
	    ("mmi test report type is %d,report_size is %d,patience=%d, status=%d\n",
	     report_type, report_size, patience, f54->status);

	if (!report_size)
		goto test_exit;
	buffer = kmalloc(report_size, GFP_KERNEL);
	if (!buffer) {
		TS_LOG_ERR("%s: Faild to kzalloc %d buffer\n", __func__,
			   report_size);
		goto test_exit;
	}
	memset(buffer, 0, report_size);

	/*load test limit*/
	/*LoadHighResistanceLimits();*/

	/*simulate ReadBlockData()*/
	mutex_lock(&f54->data_mutex);

	if (f54->report_data) {
		memcpy(buffer, f54->report_data, f54->report_size);
		mutex_unlock(&f54->data_mutex);
	} else {
		mutex_unlock(&f54->data_mutex);
		goto test_exit;
	}
	switch (report_type) {
	case F54_FULL_RAW_CAP_MIN_MAX:
		if (!g_mmi_maxmincapacitance_report)
			g_mmi_maxmincapacitance_report =
			    kmalloc(2 * DATA_TEMP_BUFF_SIZE + F54_MAX_CAP_TITLE_SIZE, GFP_KERNEL);
		mmi_maxmincapacitance_report(buffer);
		break;
	case F54_FULL_RAW_CAP_RX_COUPLING_COMP:
		if (!g_mmi_buf_f54raw_data)
			g_mmi_buf_f54raw_data =
			    kmalloc(rx * tx * DATA_TEMP_BUFF_SIZE + F54_MAX_CAP_TITLE_SIZE,
				    GFP_KERNEL);
		mmi_rawimage_report(buffer);
		break;
	case F54_TX_TO_TX_SHORT:
		mmi_txtotx_short_report(buffer);
		break;
	case F54_HIGH_RESISTANCE:
		if (!g_mmi_highresistance_report)
			g_mmi_highresistance_report =
			    kmalloc(3 * DATA_TEMP_BUFF_SIZE + F54_MAX_CAP_TITLE_SIZE, GFP_KERNEL);
		mmi_highresistance_report(buffer);
		break;
	case F54_TX_TO_GROUND:
		mmi_txtoground_short_report(buffer, report_size);
		break;
	case F54_RX_TO_RX1:
		if (!g_mmi_RxtoRxshort_report)
			g_mmi_RxtoRxshort_report =
			    kmalloc(6 * rx + F54_MAX_CAP_TITLE_SIZE,
				    GFP_KERNEL);
		mmi_RxtoRxshort1_report(buffer);
		break;
	case F54_RX_TO_RX2:
		/*g_mmi_RxtoRxshort_report = kmalloc(4536, GFP_KERNEL);*/
		mmi_RxtoRxshort2_report(buffer);
		break;
	case F54_16BIT_IMAGE:
		if (!g_buf_debug_data)
			g_buf_debug_data = kmalloc(rx * tx * DATA_TEMP_BUFF_SIZE + F54_MAX_CAP_TITLE_SIZE + 4 * F54_MAX_CAP_TITLE_SIZE, GFP_KERNEL);	/*buf len for test*/
		mmi_delta_report(buffer);
		break;
	default:
		break;
	}
	kfree(buffer);
	return 0;

test_exit:
	TS_LOG_ERR("%s: Faild to run test\n", __func__);
	if (buffer)
		kfree(buffer);
	return -EINVAL;

}

static int synaptics_rmi4_irq_enable(bool enable)
{
	int retval = 0;

	unsigned short reg_addr = Interrupt_Enable_Addr;
	unsigned char reg_data = 0;
	struct synaptics_rmi4_data *rmi4_data = f54->rmi4_data;

	TS_LOG_INFO("%s;enable =%d\n", __func__, enable);
	retval = f54->fn_ptr->read(rmi4_data,
				   reg_addr, &reg_data, sizeof(reg_data));

	if (retval < 0) {
		TS_LOG_ERR("Failed to read reg, error = %d\n", retval);
		return retval;
	}

	if (IRQ_ON == enable) {
		reg_data |= IRQ_ENA_MASK_BIT;
	} else {
		reg_data &= ~IRQ_ENA_MASK_BIT;
	}

	retval = f54->fn_ptr->write(rmi4_data,
				    reg_addr, &reg_data, sizeof(reg_data));

	if (retval < 0) {
		TS_LOG_ERR("Failed to write reg, error = %d\n", retval);
	}
	return retval;
}

static ssize_t synaptics_rmi4_f54_mmi_test_show(struct device *dev,
						struct device_attribute *attr,
						char *buf)
{
	int retval = 0;
	struct synaptics_rmi4_data *rmi4_data = f54->rmi4_data;
	TS_LOG_ERR("begin mmi test.\n");

	atomic_set(&rmi4_data->synaptics_chip_data->ts_platform_data->state, TS_MMI_CAP_TEST);
	if (!g_mmi_buf_f54test_result)
		g_mmi_buf_f54test_result = kmalloc(80, GFP_KERNEL);

	if (g_mmi_buf_f54test_result)
		memset(g_mmi_buf_f54test_result, 0, 80);
	else
		goto exit;
	retval = synaptics_rmi4_irq_enable(IRQ_OFF);
	retval = synaptics_rmi4_f54_do_preparation_set();
	if (retval) {
		TS_LOG_ERR("fail to do preparation set\n");
		goto exit;
	}
	sprintf(g_mmi_buf_f54test_result, "0P-");

	retval = mmi_runtest(F54_HIGH_RESISTANCE);
	if (retval)
		goto exit;

	retval = mmi_runtest(F54_FULL_RAW_CAP_MIN_MAX);
	if (retval)
		goto exit;

	retval = mmi_runtest(F54_FULL_RAW_CAP_RX_COUPLING_COMP);
	if (retval)
		goto exit;

	retval = mmi_runtest(F54_RX_TO_RX1);
	if (retval)
		goto exit;

	retval = mmi_runtest(F54_RX_TO_RX2);
	if (retval)
		goto exit;

	retval = mmi_runtest(F54_TX_TO_GROUND);
	if (retval)
		goto exit;

	retval = mmi_runtest(F54_TX_TO_TX_SHORT);
	if (retval)
		goto exit;
	if (0 == strlen(g_mmi_buf_f54test_result)
	    || strstr(g_mmi_buf_f54test_result, "F")) {
		strncat(g_mmi_buf_f54test_result, tp_test_failed_reason, 50);
	}

	switch (f54->rmi4_data->synaptics_chip_data->ic_type) {
	case SYNAPTICS_S3207:
		strncat(g_mmi_buf_f54test_result, "-synaptics_3207",
			strlen("-synaptics_3207"));
		break;
	case SYNAPTICS_S3350:
		strncat(g_mmi_buf_f54test_result, "-synaptics_3350",
			strlen("-synaptics_3350"));
		break;
	case SYNAPTICS_S3320:
		strncat(g_mmi_buf_f54test_result, "-synaptics_3320",
			strlen("-synaptics_3320"));
		break;
	case SYNAPTICS_S3718:
		strncat(g_mmi_buf_f54test_result, "-synaptics_3718",
			strlen("-synaptics_3718"));
		break;
	case SYNAPTICS_TD4322:
		strncat(g_mmi_buf_f54test_result, "-synaptics_4322",
			strlen("-synaptics_4322"));
		break;
	case SYNAPTICS_TD4310:
		strncat(g_mmi_buf_f54test_result, "-synaptics_4310",
			strlen("-synaptics_4310"));
		break;
	default:
		TS_LOG_ERR("failed to recognize ic_ver\n");
		break;
	}

	retval = synaptics_rmi4_irq_enable(IRQ_ON);
	sysfs_is_busy = true;
	f54->rmi4_data->reset_device(rmi4_data);
	atomic_set(&rmi4_data->synaptics_chip_data->ts_platform_data->state, TS_WORK);
	return 1;
exit:
	retval = synaptics_rmi4_irq_enable(IRQ_ON);
	sysfs_is_busy = true;
	f54->rmi4_data->reset_device(rmi4_data);
	strcat(g_mmi_buf_f54test_result, "0F-software_reason");
	TS_LOG_ERR("%s: Failed to run mmi test\n", __func__);
	atomic_set(&rmi4_data->synaptics_chip_data->ts_platform_data->state, TS_WORK);
	return 0;
}

static int read_debug_reg_status(unsigned char *buffer)
{
	int retval = 0;
	unsigned char command = 0;
	char buf[6] = { 0 };
	size_t len = 0;
	TS_LOG_ERR("read_debug_reg_status_begin\n");
	retval = f54->fn_ptr->read(f54->rmi4_data,
				   0x0051, &command, sizeof(command));
	if (retval < 0) {
		TS_LOG_ERR("%s: Failed to read control 0x0051 \n", __func__);
		strcat(g_buf_debug_data, "Failed to read control 0x0051");
	} else {
		TS_LOG_INFO("Device Control1=0x%x\n", command);
		strcat(g_buf_debug_data, "Device Control1:");
		len = rx * tx * DATA_TEMP_BUFF_SIZE + F54_MAX_CAP_TITLE_SIZE + F54_MAX_CAP_TITLE_SIZE +
		F54_MAX_CAP_TITLE_SIZE - strlen(g_buf_debug_data) -1;
		sprintf(buf, "0x%x ", command);
		strncat(g_buf_debug_data, buf, len);
	}
	retval = f54->fn_ptr->read(f54->rmi4_data,
				   0x0052, &command, sizeof(command));
	if (retval < 0) {
		TS_LOG_ERR("%s: Failed to read control 0x0052 \n", __func__);
		strcat(g_buf_debug_data, "Failed to read control 0x0052");
	} else {
		TS_LOG_INFO("Interrupt Enable=0x%x\n", command);
		strcat(g_buf_debug_data, "Interrupt Enable:");
		sprintf(buf, "0x%x ", command);
		len = rx * tx * DATA_TEMP_BUFF_SIZE + F54_MAX_CAP_TITLE_SIZE + F54_MAX_CAP_TITLE_SIZE +
		F54_MAX_CAP_TITLE_SIZE - strlen(g_buf_debug_data) -1;
		strncat(g_buf_debug_data, buf, len);
	}

	retval = f54->fn_ptr->read(f54->rmi4_data,
				   0x0013, &command, sizeof(command));
	if (retval < 0) {
		TS_LOG_ERR("%s: Failed to read control 0x0013 \n", __func__);
		strcat(g_buf_debug_data, "Failed to read control 0x0013");
	} else {
		TS_LOG_INFO("Device Status=0x%x\n", command);
		strcat(g_buf_debug_data, "Device Status:");
		sprintf(buf, "0x%x ", command);
		len = rx * tx * DATA_TEMP_BUFF_SIZE + F54_MAX_CAP_TITLE_SIZE + F54_MAX_CAP_TITLE_SIZE +
		F54_MAX_CAP_TITLE_SIZE - strlen(g_buf_debug_data) -1;
		strncat(g_buf_debug_data, buf, len);
	}

	retval = f54->fn_ptr->read(f54->rmi4_data,
				   0x0014, &command, sizeof(command));
	if (retval < 0) {
		TS_LOG_ERR("%s: Failed to read control 0x0014 \n", __func__);
		strcat(g_buf_debug_data, "Failed to read control 0x0014");
	} else {
		TS_LOG_INFO("Interrupt Status=0x%x\n", command);
		strcat(g_buf_debug_data, "Interrupt Status:");
		sprintf(buf, "0x%x ", command);
		len = rx * tx * DATA_TEMP_BUFF_SIZE + F54_MAX_CAP_TITLE_SIZE + F54_MAX_CAP_TITLE_SIZE +
		F54_MAX_CAP_TITLE_SIZE - strlen(g_buf_debug_data) -1;
		strncat(g_buf_debug_data, buf, len);
		strncat(g_buf_debug_data, "\0", 1);
	}

	return retval;
}

static int read_debug_power_pin_status(unsigned char *buffer)
{
	int retval = 0;
	char buf[10] = { 0 };
	size_t len = 0;
	retval =
	    gpio_get_value(f54->rmi4_data->synaptics_chip_data->ts_platform_data->reset_gpio);
	if (retval < 0) {
		TS_LOG_ERR("failed to get reset_gpio %d\n", retval);
		strcat(g_buf_debug_data, "failed to get reset_gpio");
	} else {
		TS_LOG_INFO("reset_gpio=%d\n", retval);
		strcat(g_buf_debug_data, "reset_gpio value is:");
		sprintf(buf, "0x%x ", retval);
		len = rx * tx * DATA_TEMP_BUFF_SIZE + F54_MAX_CAP_TITLE_SIZE + F54_MAX_CAP_TITLE_SIZE +
		F54_MAX_CAP_TITLE_SIZE - strlen(g_buf_debug_data) -1;
		strncat(g_buf_debug_data, buf, len);
	}
	retval = gpio_get_value(f54->rmi4_data->synaptics_chip_data->ts_platform_data->irq_gpio);
	if (retval < 0) {
		TS_LOG_ERR("failed to get irq_gpio %d\n", retval);
		strcat(g_buf_debug_data, "failed to get irq_gpio");
	} else {
		TS_LOG_INFO("irq_gpio=%d\n", retval);
		strcat(g_buf_debug_data, "irq_gpio value is:");
		sprintf(buf, "0x%x ", retval);
		len = rx * tx * DATA_TEMP_BUFF_SIZE + F54_MAX_CAP_TITLE_SIZE + F54_MAX_CAP_TITLE_SIZE +
		F54_MAX_CAP_TITLE_SIZE - strlen(g_buf_debug_data) -1;
		strncat(g_buf_debug_data, buf, len);
	}

	retval =
	    gpio_get_value(f54->rmi4_data->synaptics_chip_data->vci_gpio_ctrl);
	if (retval < 0) {
		TS_LOG_ERR("failed to get vci_gpio_ctrl %d\n", retval);
		strcat(g_buf_debug_data, "failed to get vci_gpio_ctrl");
	} else {
		TS_LOG_INFO("vci_gpio_ctrl=%d\n", retval);
		strcat(g_buf_debug_data, "vci_gpio_ctrl value is:");
		sprintf(buf, "0x%x ", retval);
		len = rx * tx * DATA_TEMP_BUFF_SIZE + F54_MAX_CAP_TITLE_SIZE + F54_MAX_CAP_TITLE_SIZE +
		F54_MAX_CAP_TITLE_SIZE - strlen(g_buf_debug_data) -1;
		strncat(g_buf_debug_data, buf, len);
	}

	retval =
	    gpio_get_value(f54->rmi4_data->synaptics_chip_data->
			   vddio_gpio_ctrl);
	if (retval < 0) {
		TS_LOG_ERR("failed to get vddio_gpio_ctrl %d\n", retval);
		strcat(g_buf_debug_data, "failed to get vddio_gpio_ctrl");
	} else {
		TS_LOG_INFO("vddio_gpio_ctrl=%d\n", retval);
		strcat(g_buf_debug_data, "vddio_gpio_ctrl value is:");
		sprintf(buf, "0x%x ", retval);
		len = rx * tx * DATA_TEMP_BUFF_SIZE + F54_MAX_CAP_TITLE_SIZE + F54_MAX_CAP_TITLE_SIZE +
		F54_MAX_CAP_TITLE_SIZE - strlen(g_buf_debug_data) -1;
		strncat(g_buf_debug_data, buf, len);
	}

	strcat(g_buf_debug_data, "ts_platform_data.state is:");
	sprintf(buf, "%d ", atomic_read(&rmi4_data->synaptics_chip_data->ts_platform_data->state));
	len = rx * tx * DATA_TEMP_BUFF_SIZE + F54_MAX_CAP_TITLE_SIZE + F54_MAX_CAP_TITLE_SIZE +
		F54_MAX_CAP_TITLE_SIZE - strlen(g_buf_debug_data) -1;
	strncat(g_buf_debug_data, buf, len);

	return retval;
}

static ssize_t synaptics_rmi4_f54_debug_test_show(struct device *dev,
						  struct device_attribute *attr,
						  char *buf)
{
	int retval = 0;
	int len = 0;
	struct synaptics_rmi4_data *rmi4_data = f54->rmi4_data;
	TS_LOG_INFO("begin debug test.\n");

	if ((TS_SLEEP == atomic_read(&rmi4_data->synaptics_chip_data->ts_platform_data->state))
	    || (TS_MMI_CAP_TEST == atomic_read(&rmi4_data->synaptics_chip_data->ts_platform_data->state))) {
		TS_LOG_INFO("tp state unsupport debug.ts_platform_data.state is %d\n",
			    atomic_read(&rmi4_data->synaptics_chip_data->ts_platform_data->state));
		return 0;
	}
	atomic_set(&rmi4_data->synaptics_chip_data->ts_platform_data->state, TS_MMI_CAP_TEST);
	retval = mmi_runtest(F54_16BIT_IMAGE);
	if (retval) {
		TS_LOG_ERR("fail to do F54_16BIT_IMAGE test\n");
	}

	retval = read_debug_reg_status(g_buf_debug_data);
	if (retval < 0) {
		TS_LOG_ERR("fail to do read_debug_reg_status test\n");
	}
	retval = read_debug_power_pin_status(g_buf_debug_data);
	if (retval < 0) {
		TS_LOG_ERR("fail to do read_debug_power_pin_status test\n");
	}

	sysfs_is_busy = true;
	f54->rmi4_data->reset_device(rmi4_data);

	if (NULL != g_buf_debug_data) {
		len = strlen(g_buf_debug_data);
		memcpy(buf, g_buf_debug_data, len + 1);
		strcat(buf, "\0");
		TS_LOG_INFO("%s\n", buf);
	} else {
		TS_LOG_INFO("read debug data error\n");
	}
	if (NULL != g_buf_debug_data) {
		kfree(g_buf_debug_data);
	}
	atomic_set(&rmi4_data->synaptics_chip_data->ts_platform_data->state, TS_WORK);
	return len;
}

static ssize_t synaptics_rmi4_f54_mmi_test_result_show(struct device *dev,
						       struct device_attribute
						       *attr, char *buf)
{
	int len = 0;
	TS_LOG_INFO("%s begin \n", __func__);
	if (NULL != g_mmi_buf_f54test_result) {
		len = strlen(g_mmi_buf_f54test_result);
		memcpy(buf, g_mmi_buf_f54test_result, len + 1);
		strcat(buf, "\n");
	} else
		goto exit;

	if ((NULL != g_mmi_buf_f54raw_data)
	    && (NULL != g_mmi_highresistance_report)
	    && (NULL != g_mmi_maxmincapacitance_report)
	    && (NULL != g_mmi_RxtoRxshort_report)) {
		strcat(buf, g_mmi_buf_f54raw_data);
		strcat(buf, g_mmi_highresistance_report);
		strcat(buf, g_mmi_maxmincapacitance_report);
		strcat(buf, g_mmi_RxtoRxshort_report);
		strcat(buf, "\0");

		len =
		    strlen(g_mmi_buf_f54test_result) +
		    strlen(g_mmi_buf_f54raw_data) + 1 +
		    strlen(g_mmi_highresistance_report) +
		    strlen(g_mmi_maxmincapacitance_report)
		    + strlen(g_mmi_RxtoRxshort_report);
	}
exit:
	return len;
}

static ssize_t hw_synaptics_mmi_test_show(struct kobject *dev,
					  struct kobj_attribute *attr,
					  char *buf)
{
	struct synaptics_rmi4_data *rmi4_data = f54->rmi4_data;
	struct device *cdev = &rmi4_data->input_dev->dev;
	if (!cdev) {
		TS_LOG_ERR("device is null\n");
		return -EINVAL;
	}
	return synaptics_rmi4_f54_mmi_test_show(cdev, NULL, (char *)buf);
}

static ssize_t hw_synaptics_debug_test_show(struct kobject *dev,
					    struct kobj_attribute *attr,
					    char *buf)
{
	struct synaptics_rmi4_data *rmi4_data = f54->rmi4_data;
	struct device *cdev = &rmi4_data->input_dev->dev;
	if (!cdev) {
		TS_LOG_ERR("device is null\n");
		return -EINVAL;
	}
	return synaptics_rmi4_f54_debug_test_show(cdev, NULL, (char *)buf);
}

static ssize_t hw_synaptics_trigger_log_show(struct kobject *dev,
					     struct kobj_attribute *attr,
					     char *buf)
{
	TS_LOG_ERR("g_synaptics_trigger_log_flag show is %d\n",
		   g_synaptics_trigger_log_flag);

	return sprintf((char *)buf, "%d\n", g_synaptics_trigger_log_flag);
}

static ssize_t hw_synaptics_trigger_log_store(struct kobject *dev,
					      struct kobj_attribute *attr,
					      const char *buf, size_t count)
{
	int retval = NO_ERR;
	unsigned long setting = 0;

	retval = sstrtoul(buf, 10, &setting);
	if (retval)
		return retval;

	g_synaptics_trigger_log_flag = setting;

	TS_LOG_ERR("g_synaptics_trigger_log_flag store is %d\n",
		   g_synaptics_trigger_log_flag);

	return count;
}

static ssize_t hw_synaptics_mmi_test_result_show(struct kobject *dev,
						 struct kobj_attribute *attr,
						 char *buf)
{
	struct synaptics_rmi4_data *rmi4_data = f54->rmi4_data;
	struct device *cdev = &rmi4_data->input_dev->dev;
	if (!cdev) {
		TS_LOG_ERR("device is null\n");
		return -EINVAL;
	}
	return synaptics_rmi4_f54_mmi_test_result_show(cdev, NULL, buf);
}

static struct kobj_attribute synaptics_mmi_test_v = {
	.attr = {.name = "tp_legacy_capacitance_test", .mode = 0664},
	.show = hw_synaptics_mmi_test_show,
	.store = NULL,
};

static struct kobj_attribute synaptics_mmi_test_result_v = {
	.attr = {.name = "tp_legacy_capacitance_data", .mode = 0444},
	.show = hw_synaptics_mmi_test_result_show,
	.store = NULL,
};

static struct kobj_attribute synaptics_debug_test_v = {
	.attr = {.name = "synaptics_debug_test", .mode = 0664},
	.show = hw_synaptics_debug_test_show,
	.store = NULL,
};

static struct kobj_attribute synaptics_trigger_log_v = {
	.attr = {.name = "synaptics_trigger_log", .mode = 0664},
	.show = hw_synaptics_trigger_log_show,
	.store = hw_synaptics_trigger_log_store,
};

static int add_synaptics_mmi_test_interfaces(struct device *dev)
{
	int error = 0;
	static int flag = 0;
	struct kobject *properties_kobj;

	if (0 != flag) {
		return 0;
	}

	TS_LOG_INFO("%s: in!\n", __func__);
	properties_kobj = tp_get_touch_screen_obj();
	if (NULL == properties_kobj) {
		TS_LOG_ERR("Error, get kobj failed!\n");
		return -1;
	}

	/*add the node synaptics_mmi_test_result for apk to read */
	error =
	    sysfs_create_file(properties_kobj,
			      &synaptics_mmi_test_result_v.attr);
	if (error) {
		kobject_put(properties_kobj);
		TS_LOG_ERR("synaptics_mmi_test_result create file error = %d\n",
			   error);
		return -ENODEV;
	}

	/*add the node synaptics_mmi_test apk to write */
	error = sysfs_create_file(properties_kobj, &synaptics_mmi_test_v.attr);
	if (error) {
		kobject_put(properties_kobj);
		TS_LOG_ERR("synaptics_mmi_test create file error\n");
		return -ENODEV;
	}
	error =
	    sysfs_create_file(properties_kobj, &synaptics_debug_test_v.attr);
	if (error) {
		kobject_put(properties_kobj);
		TS_LOG_ERR("synaptics_mmi_test create file error\n");
		return -ENODEV;
	}
	error =
	    sysfs_create_file(properties_kobj, &synaptics_trigger_log_v.attr);
	if (error) {
		kobject_put(properties_kobj);
		TS_LOG_ERR("synaptics_trigger_log create file error\n");
		return -ENODEV;
	}

	flag = 1;
	return 0;
}


#endif

static ssize_t synaptics_rmi4_f54_resume_touch_store(struct device *dev,
						     struct device_attribute
						     *attr, const char *buf,
						     size_t count)
{
	int retval = NO_ERR;
	unsigned long setting = 0;

	retval = sstrtoul(buf, 10, &setting);
	if (retval)
		return retval;

	if (setting != 1)
		return -EINVAL;

	/*set_interrupt(false);*/

	return count;
}

static ssize_t synaptics_rmi4_f54_num_of_mapped_rx_show(struct device *dev,
							struct device_attribute
							*attr, char *buf)
{
	return snprintf(buf, PAGE_SIZE, "%u\n", f54->rx_assigned);
}

static ssize_t synaptics_rmi4_f54_num_of_mapped_tx_show(struct device *dev,
							struct device_attribute
							*attr, char *buf)
{
	return snprintf(buf, PAGE_SIZE, "%u\n", f54->tx_assigned);
}

simple_show_func_unsigned(query, num_of_rx_electrodes)
    simple_show_func_unsigned(query, num_of_tx_electrodes)
    simple_show_func_unsigned(query, has_image16)
    simple_show_func_unsigned(query, has_image8)
    simple_show_func_unsigned(query, has_baseline)
    simple_show_func_unsigned(query, clock_rate)
    simple_show_func_unsigned(query, touch_controller_family)
    simple_show_func_unsigned(query, has_pixel_touch_threshold_adjustment)
    simple_show_func_unsigned(query, has_sensor_assignment)
    simple_show_func_unsigned(query, has_interference_metric)
    simple_show_func_unsigned(query, has_sense_frequency_control)
    simple_show_func_unsigned(query, has_firmware_noise_mitigation)
    simple_show_func_unsigned(query, has_two_byte_report_rate)
    simple_show_func_unsigned(query, has_one_byte_report_rate)
    simple_show_func_unsigned(query, has_relaxation_control)
    simple_show_func_unsigned(query, curve_compensation_mode)
    simple_show_func_unsigned(query, has_iir_filter)
    simple_show_func_unsigned(query, has_cmn_removal)
    simple_show_func_unsigned(query, has_cmn_maximum)
    simple_show_func_unsigned(query, has_touch_hysteresis)
    simple_show_func_unsigned(query, has_edge_compensation)
    simple_show_func_unsigned(query, has_per_frequency_noise_control)
    simple_show_func_unsigned(query, has_signal_clarity)
    simple_show_func_unsigned(query, number_of_sensing_frequencies)

    show_store_func_unsigned(control, reg_0, no_relax)
    show_store_func_unsigned(control, reg_0, no_scan)
    show_store_func_unsigned(control, reg_1, bursts_per_cluster)
    show_store_func_unsigned(control, reg_2, saturation_cap)
    show_store_func_unsigned(control, reg_3, pixel_touch_threshold)
    show_store_func_unsigned(control, reg_4__6, rx_feedback_cap)
    show_store_func_unsigned(control, reg_4__6, low_ref_cap)
    show_store_func_unsigned(control, reg_4__6, low_ref_feedback_cap)
    show_store_func_unsigned(control, reg_4__6, low_ref_polarity)
    show_store_func_unsigned(control, reg_4__6, high_ref_cap)
    show_store_func_unsigned(control, reg_4__6, high_ref_feedback_cap)
    show_store_func_unsigned(control, reg_4__6, high_ref_polarity)
    show_store_func_unsigned(control, reg_7, cbc_cap)
    show_store_func_unsigned(control, reg_7, cbc_polarity)
    show_store_func_unsigned(control, reg_7, cbc_tx_carrier_selection)
    show_store_func_unsigned(control, reg_8__9, integration_duration)
    show_store_func_unsigned(control, reg_8__9, reset_duration)
    show_store_func_unsigned(control, reg_10, noise_sensing_bursts_per_image)
    show_store_func_unsigned(control, reg_12__13, slow_relaxation_rate)
    show_store_func_unsigned(control, reg_12__13, fast_relaxation_rate)
    show_store_func_unsigned(control, reg_14, rxs_on_xaxis)
    show_store_func_unsigned(control, reg_14, curve_comp_on_txs)
    show_store_func_unsigned(control, reg_20, disable_noise_mitigation)
    show_store_func_unsigned(control, reg_21, freq_shift_noise_threshold)
    show_store_func_unsigned(control, reg_22__26, medium_noise_threshold)
    show_store_func_unsigned(control, reg_22__26, high_noise_threshold)
    show_store_func_unsigned(control, reg_22__26, noise_density)
    show_store_func_unsigned(control, reg_22__26, frame_count)
    show_store_func_unsigned(control, reg_27, iir_filter_coef)
    show_store_func_unsigned(control, reg_28, quiet_threshold)
    show_store_func_unsigned(control, reg_29, cmn_filter_disable)
    show_store_func_unsigned(control, reg_30, cmn_filter_max)
    show_store_func_unsigned(control, reg_31, touch_hysteresis)
    show_store_func_unsigned(control, reg_32__35, rx_low_edge_comp)
    show_store_func_unsigned(control, reg_32__35, rx_high_edge_comp)
    show_store_func_unsigned(control, reg_32__35, tx_low_edge_comp)
    show_store_func_unsigned(control, reg_32__35, tx_high_edge_comp)
    show_store_func_unsigned(control, reg_41, no_signal_clarity)
    show_store_func_unsigned(control, reg_57, cbc_cap_0d)
    show_store_func_unsigned(control, reg_57, cbc_polarity_0d)
    show_store_func_unsigned(control, reg_57, cbc_tx_carrier_selection_0d)

    show_replicated_func_unsigned(control, reg_15, sensor_rx_assignment)
    show_replicated_func_unsigned(control, reg_16, sensor_tx_assignment)
    show_replicated_func_unsigned(control, reg_17, disable)
    show_replicated_func_unsigned(control, reg_17, filter_bandwidth)
    show_replicated_func_unsigned(control, reg_19, stretch_duration)
    show_replicated_func_unsigned(control, reg_38, noise_control_1)
    show_replicated_func_unsigned(control, reg_39, noise_control_2)
    show_replicated_func_unsigned(control, reg_40, noise_control_3)

    show_store_replicated_func_unsigned(control, reg_36, axis1_comp)
    show_store_replicated_func_unsigned(control, reg_37, axis2_comp)

static ssize_t synaptics_rmi4_f54_burst_count_show(struct device *dev,
						   struct device_attribute
						   *attr, char *buf)
{
	int retval = NO_ERR;
	int size = 0;
	unsigned char ii = 0;
	unsigned char *temp = NULL;
	struct synaptics_rmi4_data *rmi4_data = f54->rmi4_data;

	mutex_lock(&f54->control_mutex);

	retval = f54->fn_ptr->read(rmi4_data,
				   f54->control.reg_17->address,
				   (unsigned char *)f54->control.reg_17->data,
				   f54->control.reg_17->length);
	if (retval < 0) {
		TS_LOG_ERR("%s: Failed to read control reg_17\n", __func__);
	}

	retval = f54->fn_ptr->read(rmi4_data,
				   f54->control.reg_18->address,
				   (unsigned char *)f54->control.reg_18->data,
				   f54->control.reg_18->length);
	if (retval < 0) {
		TS_LOG_ERR("%s: Failed to read control reg_18\n", __func__);
	}

	mutex_unlock(&f54->control_mutex);

	temp = buf;

	for (ii = 0; ii < f54->control.reg_17->length; ii++) {
		retval = snprintf(temp, PAGE_SIZE - size, "%u ", (1 << 8) *
				  f54->control.reg_17->data[ii].
				  burst_count_b8__10 +
				  f54->control.reg_18->data[ii].
				  burst_count_b0__7);
		if (retval < 0) {
			TS_LOG_ERR("%s: Faild to write output\n", __func__);
			return retval;
		}
		size += retval;
		temp += retval;
	}

	retval = snprintf(temp, PAGE_SIZE - size, "\n");
	if (retval < 0) {
		TS_LOG_ERR("%s: Faild to write null terminator\n", __func__);
		return retval;
	}

	return size + retval;
}

static ssize_t synaptics_rmi4_f54_data_read(struct file *data_file,
					    struct kobject *kobj,
					    struct bin_attribute *attributes,
					    char *buf, loff_t pos, size_t count)
{
	/*struct synaptics_rmi4_data *rmi4_data = f54->rmi4_data;*/

	mutex_lock(&f54->data_mutex);

	if (count < f54->report_size) {
		TS_LOG_ERR("%s: Report type %d data size (%d) too large\n",
			   __func__, f54->report_type, f54->report_size);
		mutex_unlock(&f54->data_mutex);
		return -EINVAL;
	}

	if (f54->report_data) {
		memcpy(buf, f54->report_data, f54->report_size);
		mutex_unlock(&f54->data_mutex);
		return f54->report_size;
	} else {
		TS_LOG_ERR("%s: Report type %d data not available\n", __func__,
			   f54->report_type);
		mutex_unlock(&f54->data_mutex);
		return -EINVAL;
	}
}

static int synaptics_rmi4_f54_set_sysfs(void)
{
	int retval = NO_ERR;
	int reg_num = 0;
	struct synaptics_rmi4_data *rmi4_data = f54->rmi4_data;

	TS_LOG_INFO("%s: in!\n", __func__);
	f54->attr_dir = kobject_create_and_add("f54",
					       &rmi4_data->input_dev->dev.kobj);
	if (!f54->attr_dir) {
		TS_LOG_ERR("%s: Failed to create sysfs directory\n", __func__);
		goto exit_1;
	}

	retval = sysfs_create_bin_file(f54->attr_dir, &dev_report_data);
	if (retval < 0) {
		TS_LOG_ERR("%s: Failed to create sysfs bin file\n", __func__);
		goto exit_2;
	}

	retval = sysfs_create_group(f54->attr_dir, &attr_group);
	if (retval < 0) {
		TS_LOG_ERR("%s: Failed to create sysfs attributes\n", __func__);
		goto exit_3;
	}

	for (reg_num = 0; reg_num < ARRAY_SIZE(attrs_ctrl_regs); reg_num++) {
		if (attrs_ctrl_regs_exist[reg_num]) {
			retval = sysfs_create_group(f54->attr_dir,
						    &attrs_ctrl_regs[reg_num]);
			if (retval < 0) {
				TS_LOG_ERR
				    ("%s: Failed to create sysfs attributes\n",
				     __func__);
				goto exit_4;
			}
		}
	}

	retval = add_synaptics_mmi_test_interfaces(&rmi4_data->input_dev->dev);
	if (retval < 0) {
		TS_LOG_ERR("Error, synaptics_mmi_test init sysfs fail! \n");
		goto exit_4;
	}

	return 0;

exit_4:
	sysfs_remove_group(f54->attr_dir, &attr_group);

	for (reg_num--; reg_num >= 0; reg_num--)
		sysfs_remove_group(f54->attr_dir, &attrs_ctrl_regs[reg_num]);

exit_3:
	sysfs_remove_bin_file(f54->attr_dir, &dev_report_data);

exit_2:
	kobject_put(f54->attr_dir);

exit_1:
	return -ENODEV;
}

static int synaptics_rmi4_f54_set_ctrl(void)
{
	unsigned char length = 0;
	unsigned char reg_num = 0;
	unsigned char num_of_sensing_freqs = 0;
	unsigned short reg_addr = f54->control_base_addr;
	struct f54_control *control = &f54->control;
	/*struct synaptics_rmi4_data *rmi4_data = f54->rmi4_data;*/

	num_of_sensing_freqs = f54->query.number_of_sensing_frequencies;

	/* control 0 */
	attrs_ctrl_regs_exist[reg_num] = true;
	control->reg_0 = kzalloc(sizeof(*(control->reg_0)), GFP_KERNEL);
	if (!control->reg_0)
		goto exit_no_mem;
	control->reg_0->address = reg_addr;
	reg_addr += sizeof(control->reg_0->data);
	reg_num++;

	/* control 1 */
	if ((f54->query.touch_controller_family == 0) ||
	    (f54->query.touch_controller_family == 1)) {
		attrs_ctrl_regs_exist[reg_num] = true;
		control->reg_1 = kzalloc(sizeof(*(control->reg_1)), GFP_KERNEL);
		if (!control->reg_1)
			goto exit_no_mem;
		control->reg_1->address = reg_addr;
		reg_addr += sizeof(control->reg_1->data);
	}
	reg_num++;

	/* control 2 */
	attrs_ctrl_regs_exist[reg_num] = true;
	control->reg_2 = kzalloc(sizeof(*(control->reg_2)), GFP_KERNEL);
	if (!control->reg_2)
		goto exit_no_mem;
	control->reg_2->address = reg_addr;
	reg_addr += sizeof(control->reg_2->data);
	reg_num++;

	/* control 3 */
	if (f54->query.has_pixel_touch_threshold_adjustment == 1) {
		attrs_ctrl_regs_exist[reg_num] = true;
		control->reg_3 = kzalloc(sizeof(*(control->reg_3)), GFP_KERNEL);
		if (!control->reg_3)
			goto exit_no_mem;
		control->reg_3->address = reg_addr;
		reg_addr += sizeof(control->reg_3->data);
	}
	reg_num++;

	/* controls 4 5 6 */
	if ((f54->query.touch_controller_family == 0) ||
	    (f54->query.touch_controller_family == 1)) {
		attrs_ctrl_regs_exist[reg_num] = true;
		control->reg_4__6 = kzalloc(sizeof(*(control->reg_4__6)),
					    GFP_KERNEL);
		if (!control->reg_4__6)
			goto exit_no_mem;
		control->reg_4__6->address = reg_addr;
		reg_addr += sizeof(control->reg_4__6->data);
	}
	reg_num++;

	/* control 7 */
	if (f54->query.touch_controller_family == 1) {
		attrs_ctrl_regs_exist[reg_num] = true;
		control->reg_7 = kzalloc(sizeof(*(control->reg_7)), GFP_KERNEL);
		if (!control->reg_7)
			goto exit_no_mem;
		control->reg_7->address = reg_addr;
		reg_addr += sizeof(control->reg_7->data);
	}
	reg_num++;

	/* controls 8 9 */
	if ((f54->query.touch_controller_family == 0) ||
	    (f54->query.touch_controller_family == 1)) {
		attrs_ctrl_regs_exist[reg_num] = true;
		control->reg_8__9 = kzalloc(sizeof(*(control->reg_8__9)),
					    GFP_KERNEL);
		if (!control->reg_8__9)
			goto exit_no_mem;
		control->reg_8__9->address = reg_addr;
		reg_addr += sizeof(control->reg_8__9->data);
	}
	reg_num++;

	/* control 10 */
	if (f54->query.has_interference_metric == 1) {
		attrs_ctrl_regs_exist[reg_num] = true;
		control->reg_10 = kzalloc(sizeof(*(control->reg_10)),
					  GFP_KERNEL);
		if (!control->reg_10)
			goto exit_no_mem;
		control->reg_10->address = reg_addr;
		reg_addr += sizeof(control->reg_10->data);
	}
	reg_num++;

	/* control 11 */
	if (f54->query.has_ctrl11 == 1) {
		attrs_ctrl_regs_exist[reg_num] = true;
		control->reg_11 = kzalloc(sizeof(*(control->reg_11)),
					  GFP_KERNEL);
		if (!control->reg_11)
			goto exit_no_mem;
		control->reg_11->address = reg_addr;
		reg_addr += sizeof(control->reg_11->data);
	}
	reg_num++;

	/* controls 12 13 */
	if (f54->query.has_relaxation_control == 1) {
		attrs_ctrl_regs_exist[reg_num] = true;
		control->reg_12__13 = kzalloc(sizeof(*(control->reg_12__13)),
					      GFP_KERNEL);
		if (!control->reg_12__13)
			goto exit_no_mem;
		control->reg_12__13->address = reg_addr;
		reg_addr += sizeof(control->reg_12__13->data);
	}
	reg_num++;

	/* controls 14 15 16 */
	if (f54->query.has_sensor_assignment == 1) {
		attrs_ctrl_regs_exist[reg_num] = true;

		control->reg_14 = kzalloc(sizeof(*(control->reg_14)),
					  GFP_KERNEL);
		if (!control->reg_14)
			goto exit_no_mem;
		control->reg_14->address = reg_addr;
		reg_addr += sizeof(control->reg_14->data);

		control->reg_15 = kzalloc(sizeof(*(control->reg_15)),
					  GFP_KERNEL);
		if (!control->reg_15)
			goto exit_no_mem;
		control->reg_15->length = f54->query.num_of_rx_electrodes;
		control->reg_15->data = kzalloc(control->reg_15->length *
						sizeof(*
						       (control->reg_15->data)),
						GFP_KERNEL);
		if (!control->reg_15->data)
			goto exit_no_mem;
		control->reg_15->address = reg_addr;
		reg_addr += control->reg_15->length;

		control->reg_16 = kzalloc(sizeof(*(control->reg_16)),
					  GFP_KERNEL);
		if (!control->reg_16)
			goto exit_no_mem;
		control->reg_16->length = f54->query.num_of_tx_electrodes;
		control->reg_16->data = kzalloc(control->reg_16->length *
						sizeof(*
						       (control->reg_16->data)),
						GFP_KERNEL);
		if (!control->reg_16->data)
			goto exit_no_mem;
		control->reg_16->address = reg_addr;
		reg_addr += control->reg_16->length;
	}
	reg_num++;

	/* controls 17 18 19 */
	if (f54->query.has_sense_frequency_control == 1) {
		attrs_ctrl_regs_exist[reg_num] = true;

		length = num_of_sensing_freqs;

		control->reg_17 = kzalloc(sizeof(*(control->reg_17)),
					  GFP_KERNEL);
		if (!control->reg_17)
			goto exit_no_mem;
		control->reg_17->length = length;
		control->reg_17->data = kzalloc(length *
						sizeof(*
						       (control->reg_17->data)),
						GFP_KERNEL);
		if (!control->reg_17->data)
			goto exit_no_mem;
		control->reg_17->address = reg_addr;
		reg_addr += length;

		control->reg_18 = kzalloc(sizeof(*(control->reg_18)),
					  GFP_KERNEL);
		if (!control->reg_18)
			goto exit_no_mem;
		control->reg_18->length = length;
		control->reg_18->data = kzalloc(length *
						sizeof(*
						       (control->reg_18->data)),
						GFP_KERNEL);
		if (!control->reg_18->data)
			goto exit_no_mem;
		control->reg_18->address = reg_addr;
		reg_addr += length;

		control->reg_19 = kzalloc(sizeof(*(control->reg_19)),
					  GFP_KERNEL);
		if (!control->reg_19)
			goto exit_no_mem;
		control->reg_19->length = length;
		control->reg_19->data = kzalloc(length *
						sizeof(*
						       (control->reg_19->data)),
						GFP_KERNEL);
		if (!control->reg_19->data)
			goto exit_no_mem;
		control->reg_19->address = reg_addr;
		reg_addr += length;
	}
	reg_num++;

	/* control 20 */
	attrs_ctrl_regs_exist[reg_num] = true;
	control->reg_20 = kzalloc(sizeof(*(control->reg_20)), GFP_KERNEL);
	if (!control->reg_20)
		goto exit_no_mem;
	control->reg_20->address = reg_addr;
	reg_addr += sizeof(control->reg_20->data);
	reg_num++;

	/* control 21 */
	if (f54->query.has_sense_frequency_control == 1) {
		attrs_ctrl_regs_exist[reg_num] = true;
		control->reg_21 = kzalloc(sizeof(*(control->reg_21)),
					  GFP_KERNEL);
		if (!control->reg_21)
			goto exit_no_mem;
		control->reg_21->address = reg_addr;
		reg_addr += sizeof(control->reg_21->data);
	}
	reg_num++;

	/* controls 22 23 24 25 26 */
	if (f54->query.has_firmware_noise_mitigation == 1) {
		attrs_ctrl_regs_exist[reg_num] = true;
		control->reg_22__26 = kzalloc(sizeof(*(control->reg_22__26)),
					      GFP_KERNEL);
		if (!control->reg_22__26)
			goto exit_no_mem;
		control->reg_22__26->address = reg_addr;
		reg_addr += sizeof(control->reg_22__26->data);
	}
	reg_num++;

	/* control 27 */
	if (f54->query.has_iir_filter == 1) {
		attrs_ctrl_regs_exist[reg_num] = true;
		control->reg_27 = kzalloc(sizeof(*(control->reg_27)),
					  GFP_KERNEL);
		if (!control->reg_27)
			goto exit_no_mem;
		control->reg_27->address = reg_addr;
		reg_addr += sizeof(control->reg_27->data);
	}
	reg_num++;

	/* control 28 */
	if (f54->query.has_firmware_noise_mitigation == 1) {
		attrs_ctrl_regs_exist[reg_num] = true;
		control->reg_28 = kzalloc(sizeof(*(control->reg_28)),
					  GFP_KERNEL);
		if (!control->reg_28)
			goto exit_no_mem;
		control->reg_28->address = reg_addr;
		reg_addr += sizeof(control->reg_28->data);
	}
	reg_num++;

	/* control 29 */
	if (f54->query.has_cmn_removal == 1) {
		attrs_ctrl_regs_exist[reg_num] = true;
		control->reg_29 = kzalloc(sizeof(*(control->reg_29)),
					  GFP_KERNEL);
		if (!control->reg_29)
			goto exit_no_mem;
		control->reg_29->address = reg_addr;
		reg_addr += sizeof(control->reg_29->data);
	}
	reg_num++;

	/* control 30 */
	if (f54->query.has_cmn_maximum == 1) {
		attrs_ctrl_regs_exist[reg_num] = true;
		control->reg_30 = kzalloc(sizeof(*(control->reg_30)),
					  GFP_KERNEL);
		if (!control->reg_30)
			goto exit_no_mem;
		control->reg_30->address = reg_addr;
		reg_addr += sizeof(control->reg_30->data);
	}
	reg_num++;

	/* control 31 */
	if (f54->query.has_touch_hysteresis == 1) {
		attrs_ctrl_regs_exist[reg_num] = true;
		control->reg_31 = kzalloc(sizeof(*(control->reg_31)),
					  GFP_KERNEL);
		if (!control->reg_31)
			goto exit_no_mem;
		control->reg_31->address = reg_addr;
		reg_addr += sizeof(control->reg_31->data);
	}
	reg_num++;

	/* controls 32 33 34 35 */
	if (f54->query.has_edge_compensation == 1) {
		attrs_ctrl_regs_exist[reg_num] = true;
		control->reg_32__35 = kzalloc(sizeof(*(control->reg_32__35)),
					      GFP_KERNEL);
		if (!control->reg_32__35)
			goto exit_no_mem;
		control->reg_32__35->address = reg_addr;
		reg_addr += sizeof(control->reg_32__35->data);
	}
	reg_num++;

	/* control 36 */
	if ((f54->query.curve_compensation_mode == 1) ||
	    (f54->query.curve_compensation_mode == 2)) {
		attrs_ctrl_regs_exist[reg_num] = true;

		if (f54->query.curve_compensation_mode == 1) {
			length = max(f54->query.num_of_rx_electrodes,
				     f54->query.num_of_tx_electrodes);
		} else if (f54->query.curve_compensation_mode == 2) {
			length = f54->query.num_of_rx_electrodes;
		}

		control->reg_36 = kzalloc(sizeof(*(control->reg_36)),
					  GFP_KERNEL);
		if (!control->reg_36)
			goto exit_no_mem;
		control->reg_36->length = length;
		control->reg_36->data = kzalloc(length *
						sizeof(*
						       (control->reg_36->data)),
						GFP_KERNEL);
		if (!control->reg_36->data)
			goto exit_no_mem;
		control->reg_36->address = reg_addr;
		reg_addr += length;
	}
	reg_num++;

	/* control 37 */
	if (f54->query.curve_compensation_mode == 2) {
		attrs_ctrl_regs_exist[reg_num] = true;

		control->reg_37 = kzalloc(sizeof(*(control->reg_37)),
					  GFP_KERNEL);
		if (!control->reg_37)
			goto exit_no_mem;
		control->reg_37->length = f54->query.num_of_tx_electrodes;
		control->reg_37->data = kzalloc(control->reg_37->length *
						sizeof(*
						       (control->reg_37->data)),
						GFP_KERNEL);
		if (!control->reg_37->data)
			goto exit_no_mem;

		control->reg_37->address = reg_addr;
		reg_addr += control->reg_37->length;
	}
	reg_num++;

	/* controls 38 39 40 */
	if (f54->query.has_per_frequency_noise_control == 1) {
		attrs_ctrl_regs_exist[reg_num] = true;

		control->reg_38 = kzalloc(sizeof(*(control->reg_38)),
					  GFP_KERNEL);
		if (!control->reg_38)
			goto exit_no_mem;
		control->reg_38->length = num_of_sensing_freqs;
		control->reg_38->data = kzalloc(control->reg_38->length *
						sizeof(*
						       (control->reg_38->data)),
						GFP_KERNEL);
		if (!control->reg_38->data)
			goto exit_no_mem;
		control->reg_38->address = reg_addr;
		reg_addr += control->reg_38->length;

		control->reg_39 = kzalloc(sizeof(*(control->reg_39)),
					  GFP_KERNEL);
		if (!control->reg_39)
			goto exit_no_mem;
		control->reg_39->length = num_of_sensing_freqs;
		control->reg_39->data = kzalloc(control->reg_39->length *
						sizeof(*
						       (control->reg_39->data)),
						GFP_KERNEL);
		if (!control->reg_39->data)
			goto exit_no_mem;
		control->reg_39->address = reg_addr;
		reg_addr += control->reg_39->length;

		control->reg_40 = kzalloc(sizeof(*(control->reg_40)),
					  GFP_KERNEL);
		if (!control->reg_40)
			goto exit_no_mem;
		control->reg_40->length = num_of_sensing_freqs;
		control->reg_40->data = kzalloc(control->reg_40->length *
						sizeof(*
						       (control->reg_40->data)),
						GFP_KERNEL);
		if (!control->reg_40->data)
			goto exit_no_mem;
		control->reg_40->address = reg_addr;
		reg_addr += control->reg_40->length;
	}
	reg_num++;

	/* control 41 */
	if (f54->query.has_signal_clarity == 1) {
		attrs_ctrl_regs_exist[reg_num] = true;
		control->reg_41 = kzalloc(sizeof(*(control->reg_41)),
					  GFP_KERNEL);
		if (!control->reg_41)
			goto exit_no_mem;
		control->reg_41->address = reg_addr;
		reg_addr += sizeof(control->reg_41->data);
	}
	reg_num++;

	/* control 42 */
	if (f54->query.has_variance_metric == 1)
		reg_addr += CONTROL_42_SIZE;

	/* controls 43 44 45 46 47 48 49 50 51 52 53 54 */
	if (f54->query.has_multi_metric_state_machine == 1)
		reg_addr += CONTROL_43_54_SIZE;

	/* controls 55 56 */
	if (f54->query.has_0d_relaxation_control == 1)
		reg_addr += CONTROL_55_56_SIZE;

	/* control 57 */
	if (f54->query.has_0d_acquisition_control == 1) {
		attrs_ctrl_regs_exist[reg_num] = true;
		control->reg_57 = kzalloc(sizeof(*(control->reg_57)),
					  GFP_KERNEL);
		if (!control->reg_57)
			goto exit_no_mem;
		control->reg_57->address = reg_addr;
		reg_addr += sizeof(control->reg_57->data);
	}
	reg_num++;

	/* control 58 */
	if (f54->query.has_0d_acquisition_control == 1)
		reg_addr += CONTROL_58_SIZE;

	/* control 59 */
	if (f54->query.has_h_blank == 1)
		reg_addr += CONTROL_59_SIZE;

	/* controls 60 61 62 */
	if ((f54->query.has_h_blank == 1) ||
	    (f54->query.has_v_blank == 1) || (f54->query.has_long_h_blank == 1))
		reg_addr += CONTROL_60_62_SIZE;

	/* control 63 */
	if ((f54->query.has_h_blank == 1) ||
	    (f54->query.has_v_blank == 1) ||
	    (f54->query.has_long_h_blank == 1) ||
	    (f54->query.has_slew_metric == 1) ||
	    (f54->query.has_slew_option == 1) ||
	    (f54->query.has_noise_mitigation2 == 1))
		reg_addr += CONTROL_63_SIZE;

	/* controls 64 65 66 67 */
	if (f54->query.has_h_blank == 1)
		reg_addr += CONTROL_64_67_SIZE * 7;
	else if ((f54->query.has_v_blank == 1) ||
		 (f54->query.has_long_h_blank == 1))
		reg_addr += CONTROL_64_67_SIZE;

	/* controls 68 69 70 71 72 73 */
	if ((f54->query.has_h_blank == 1) ||
	    (f54->query.has_v_blank == 1) || (f54->query.has_long_h_blank == 1))
		reg_addr += CONTROL_68_73_SIZE;

	/* control 74 */
	if (f54->query.has_slew_metric == 1)
		reg_addr += CONTROL_74_SIZE;

	/* control 75 */
	if (f54->query.has_enhanced_stretch == 1)
		reg_addr += num_of_sensing_freqs;

	/* control 76 */
	if (f54->query.has_startup_fast_relaxation == 1)
		reg_addr += CONTROL_76_SIZE;

	/* controls 77 78 */
	if (f54->query.has_esd_control == 1)
		reg_addr += CONTROL_77_78_SIZE;

	/* controls 79 80 81 82 83 */
	if (f54->query.has_noise_mitigation2 == 1)
		reg_addr += CONTROL_79_83_SIZE;

	/* controls 84 85 */
	if (f54->query.has_energy_ratio_relaxation == 1)
		reg_addr += CONTROL_84_85_SIZE;

	/* control 86 */
	if ((f54->query.has_query13 == 1) && (f54->query.has_ctrl86 == 1))
		reg_addr += CONTROL_86_SIZE;

	/* control 87 */
	if ((f54->query.has_query13 == 1) && (f54->query.has_ctrl87 == 1))
		reg_addr += CONTROL_87_SIZE;

	/* control 88 */
	if (f54->query.has_ctrl88 == 1) {
		control->reg_88 = kzalloc(sizeof(*(control->reg_88)),
					  GFP_KERNEL);
		if (!control->reg_88)
			goto exit_no_mem;
		control->reg_88->address = reg_addr;
		/*warn repprted in hisi fortify_report check,delete it*/
		/*reg_addr += sizeof(control->reg_88->data);*/
	}

	return 0;

exit_no_mem:
	TS_LOG_ERR("%s: Failed to alloc mem for control registers\n", __func__);
	return -ENOMEM;
}

static int synaptics_rmi4_f54_status_work(struct work_struct *work)
{
	int retval = NO_ERR;
	unsigned char report_index[2] = { 0 };
	struct synaptics_rmi4_data *rmi4_data = f54->rmi4_data;
	int i = 0;
	unsigned int report_times_max = 0;
	unsigned int report_size_temp = MAX_I2C_MSG_LENS;
	unsigned char *report_data_temp = NULL;
	if (f54->status != STATUS_BUSY)
		return -EINVAL;

	set_report_size();
	if (f54->report_size == 0) {
		TS_LOG_ERR("%s: Report data size = 0\n", __func__);
		retval = -EINVAL;
		goto error_exit;
	}

	if (f54->data_buffer_size < f54->report_size) {
		mutex_lock(&f54->data_mutex);
		if (f54->data_buffer_size)
			kfree(f54->report_data);
		f54->report_data = kzalloc(f54->report_size, GFP_KERNEL);
		if (!f54->report_data) {
			TS_LOG_ERR("%s: Failed to alloc mem for data buffer\n",
				   __func__);
			f54->data_buffer_size = 0;
			mutex_unlock(&f54->data_mutex);
			retval = -ENOMEM;
			goto error_exit;
		}
		f54->data_buffer_size = f54->report_size;
		mutex_unlock(&f54->data_mutex);
	}
	report_times_max = f54->report_size / MAX_I2C_MSG_LENS;
	if (f54->report_size % MAX_I2C_MSG_LENS != 0) {
		report_times_max += 1;
	}
	report_index[0] = 0;
	report_index[1] = 0;

	retval = f54->fn_ptr->write(rmi4_data,
				    f54->data_base_addr +
				    DATA_REPORT_INDEX_OFFSET, report_index,
				    sizeof(report_index));
	if (retval < 0) {
		TS_LOG_ERR("%s: Failed to write report data index\n", __func__);
		retval = -EINVAL;
		goto error_exit;
	}
	mutex_lock(&f54->data_mutex);
	/* Point to the block data about to transfer */
	report_data_temp = f54->report_data;
	for (i = 0; i < report_times_max; i++) {

		if (i == (report_times_max - 1)) {
			/* The last time transfer the rest of the block data */
			report_size_temp = f54->report_size % MAX_I2C_MSG_LENS;
		}
		retval = f54->fn_ptr->read(rmi4_data,
					   f54->data_base_addr +
					   DATA_REPORT_DATA_OFFSET,
					   report_data_temp, report_size_temp);
		if (retval < 0) {
			TS_LOG_ERR("%s: Failed to read report data\n",
				   __func__);
			retval = -EINVAL;
			mutex_unlock(&f54->data_mutex);
			goto error_exit;
		}
		/* Point to the next 256bytes data */
		report_data_temp += MAX_I2C_MSG_LENS;
	}
	mutex_unlock(&f54->data_mutex);
	retval = STATUS_IDLE;

#ifdef RAW_HEX
	print_raw_hex_report();
#endif

#ifdef HUMAN_READABLE
	print_image_report();
#endif

error_exit:
	mutex_lock(&f54->status_mutex);
	f54->status = retval;
	mutex_unlock(&f54->status_mutex);

	return retval;
}

static int synaptics_rmi4_f54_attention(void)
{
	return synaptics_rmi4_f54_status_work(NULL);
}

static void synaptics_rmi4_f54_set_regs(struct synaptics_rmi4_data *rmi4_data,
					struct synaptics_rmi4_fn_desc *fd,
					unsigned int intr_count,
					unsigned char page)
{
	unsigned char ii = 0;
	unsigned char intr_offset = 0;

	f54->query_base_addr = fd->query_base_addr | (page << 8);
	f54->control_base_addr = fd->ctrl_base_addr | (page << 8);
	f54->data_base_addr = fd->data_base_addr | (page << 8);
	f54->command_base_addr = fd->cmd_base_addr | (page << 8);

	f54->intr_reg_num = (intr_count + 7) / 8;
	if (f54->intr_reg_num != 0)
		f54->intr_reg_num -= 1;

	f54->intr_mask = 0;
	intr_offset = intr_count % 8;
	for (ii = intr_offset;
	     ii < ((fd->intr_src_count & MASK_3BIT) + intr_offset); ii++) {
		f54->intr_mask |= 1 << ii;
	}

	return;
}

static void synaptics_rmi5_f55_init(struct synaptics_rmi4_data *rmi4_data)
{
	int retval = NO_ERR;
	unsigned char ii = 0;
	unsigned char rx_electrodes = f54->query.num_of_rx_electrodes;
	unsigned char tx_electrodes = f54->query.num_of_tx_electrodes;

	retval = f54->fn_ptr->read(rmi4_data,
				   f55->query_base_addr,
				   f55->query.data, sizeof(f55->query.data));
	if (retval < 0) {
		TS_LOG_ERR("%s: Failed to read f55 query registers\n",
			   __func__);
		return;
	}

	if (!f55->query.has_sensor_assignment)
		return;

	f55->rx_assignment = kzalloc(rx_electrodes, GFP_KERNEL);
	if (f55->rx_assignment == NULL) {
		TS_LOG_ERR("%s:kzalloc failed\n",__func__);
		return;
	}
	f55->tx_assignment = kzalloc(tx_electrodes, GFP_KERNEL);
	if (f55->tx_assignment == NULL) {
		TS_LOG_ERR("%s:kzalloc failed\n",__func__);
		return;
	}
	retval = f54->fn_ptr->read(rmi4_data,
				   f55->control_base_addr +
				   SENSOR_RX_MAPPING_OFFSET, f55->rx_assignment,
				   rx_electrodes);
	if (retval < 0) {
		TS_LOG_ERR("%s: Failed to read f55 rx assignment\n", __func__);
		return;
	}

	retval = f54->fn_ptr->read(rmi4_data,
				   f55->control_base_addr +
				   SENSOR_TX_MAPPING_OFFSET, f55->tx_assignment,
				   tx_electrodes);
	if (retval < 0) {
		TS_LOG_ERR("%s: Failed to read f55 tx assignment\n", __func__);
		return;
	}

	f54->rx_assigned = 0;
	for (ii = 0; ii < rx_electrodes; ii++) {
		if (f55->rx_assignment[ii] != 0xff)
			f54->rx_assigned++;
	}

	f54->tx_assigned = 0;
	for (ii = 0; ii < tx_electrodes; ii++) {
		if (f55->tx_assignment[ii] != 0xff)
			f54->tx_assigned++;
	}

	return;
}

static void synaptics_rmi4_f55_set_regs(struct synaptics_rmi4_data *rmi4_data,
					struct synaptics_rmi4_fn_desc *fd,
					unsigned char page)
{
	f55 = kzalloc(sizeof(*f55), GFP_KERNEL);
	if (!f55) {
		TS_LOG_ERR("%s: Failed to alloc mem for f55\n", __func__);
		return;
	}

	f55->query_base_addr = fd->query_base_addr | (page << 8);
	f55->control_base_addr = fd->ctrl_base_addr | (page << 8);
	f55->data_base_addr = fd->data_base_addr | (page << 8);
	f55->command_base_addr = fd->cmd_base_addr | (page << 8);

	return;
}

int synap_rmi4_f54_s3207_init(struct synaptics_rmi4_data *rmi4_data,
				  const char *module_name)
{
	int retval = NO_ERR;
	unsigned short ii = 0;
	unsigned char page = 0;
	unsigned char intr_count = 0;
	bool f54found = false;
	bool f55found = false;
	struct synaptics_rmi4_fn_desc rmi_fd;
	TS_LOG_INFO("%s: enter\n", __func__);

	f54 = kzalloc(sizeof(*f54), GFP_KERNEL);
	if (!f54) {
		TS_LOG_ERR("%s: Failed to alloc mem for f54\n", __func__);
		retval = -ENOMEM;
		goto exit;
	}

	f54->fn_ptr = kzalloc(sizeof(*(f54->fn_ptr)), GFP_KERNEL);
	if (!f54->fn_ptr) {
		TS_LOG_ERR("%s: Failed to alloc mem for fn_ptr\n", __func__);
		retval = -ENOMEM;
		goto exit_free_f54;
	}

	f54->rmi4_data = rmi4_data;
	f54->fn_ptr->read = rmi4_data->i2c_read;
	f54->fn_ptr->write = rmi4_data->i2c_write;
	f54->fn_ptr->enable = rmi4_data->irq_enable;

	for (page = 0; page < PAGES_TO_SERVICE; page++) {
		for (ii = PDT_START; ii > PDT_END; ii -= PDT_ENTRY_SIZE) {
			ii |= (page << 8);

			retval = f54->fn_ptr->read(rmi4_data,
						   ii,
						   (unsigned char *)&rmi_fd,
						   sizeof(rmi_fd));
			if (retval < 0)
				goto exit_free_mem;

			if (!rmi_fd.fn_number)
				break;

			switch (rmi_fd.fn_number) {
			case SYNAPTICS_RMI4_F54:
				synaptics_rmi4_f54_set_regs(rmi4_data,
							    &rmi_fd, intr_count,
							    page);
				f54found = true;
				break;
			case SYNAPTICS_RMI4_F55:
				synaptics_rmi4_f55_set_regs(rmi4_data,
							    &rmi_fd, page);
				f55found = true;
				break;
			default:
				break;
			}

			if (f54found && f55found)
				goto pdt_done;

			intr_count += (rmi_fd.intr_src_count & MASK_3BIT);
		}
	}

	if (!f54found) {
		TS_LOG_INFO("%s: line(%d)\n", __func__, __LINE__);
		retval = -ENODEV;
		goto exit_free_mem;
	}

pdt_done:
	retval = f54->fn_ptr->read(rmi4_data,
				   f54->query_base_addr,
				   f54->query.data, sizeof(f54->query.data));
	if (retval < 0) {
		TS_LOG_ERR("%s: Failed to read f54 query registers\n",
			   __func__);
		goto exit_free_mem;
	}

	f54->rx_assigned = f54->query.num_of_rx_electrodes;
	f54->tx_assigned = f54->query.num_of_tx_electrodes;

	retval = synaptics_rmi4_f54_set_ctrl();
	if (retval < 0) {
		TS_LOG_ERR("%s: Failed to set up f54 control registers\n",
			   __func__);
		goto exit_free_control;
	}

	if (f55found)
		synaptics_rmi5_f55_init(rmi4_data);

	mutex_init(&f54->status_mutex);
	mutex_init(&f54->data_mutex);
	mutex_init(&f54->control_mutex);
/*Add synaptics capacitor test function */
#if 1
	TS_LOG_INFO("%s: sysfs_is_busy = %d\n", __func__, sysfs_is_busy);
	if (!sysfs_is_busy) {
		retval = synaptics_rmi4_f54_set_sysfs();
		if (retval < 0) {
			TS_LOG_ERR("%s: Failed to create sysfs entries\n",
				   __func__);
			goto exit_sysfs;
		}
		sysfs_is_busy = false;
	}
#endif
	f54->status_workqueue =
	    create_singlethread_workqueue("f54_status_workqueue");
	INIT_DELAYED_WORK(&f54->status_work, synaptics_rmi4_f54_status_work);

#ifdef WATCHDOG_HRTIMER
	/* Watchdog timer to catch unanswered get report commands */
	hrtimer_init(&f54->watchdog, CLOCK_MONOTONIC, HRTIMER_MODE_REL);
	f54->watchdog.function = get_report_timeout;

	/* Work function to do actual cleaning up */
	INIT_WORK(&f54->timeout_work, timeout_set_status);
#endif

	f54->status = STATUS_IDLE;

	return 0;

exit_sysfs:
	kfree(f55->rx_assignment);
	kfree(f55->tx_assignment);

exit_free_control:
	free_control_mem();

exit_free_mem:
	kfree(f55);
	kfree(f54->fn_ptr);

exit_free_f54:
	kfree(f54);
	f54 = NULL;

exit:
	return retval;
}

