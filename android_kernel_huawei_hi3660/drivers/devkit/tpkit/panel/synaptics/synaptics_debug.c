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
#if defined (CONFIG_HUAWEI_DSM)
#include <dsm/dsm_pub.h>
#endif
#include "../../huawei_ts_kit.h"
#include "raw_data.h"

#define  CSV_TP_CAP_RAW_MIN "MutualRawMin"   /*1p/f*/
#define  CSV_TP_CAP_RAW_MAX "MutualRawMax"  /*1p/f*/
#define  CSV_TP_DELTA_ABS_TX_LIMIT "Tx_delta_abslimit"
#define  CSV_TP_DELTA_ABS_RX_LIMIT "Rx_delta_abslimit"
#define  CSV_TP_NOISE_DATA_LIMIT "noise_data_limit"
#define  CSV_TP_EE_SHORT_DATA_LIMIT "tddi_ee_short_data_limit"
#define  CSV_TP_SELFCAP_RAW_TX_RANGE "SelfCapRawTxRange"    /*6p/f*/
#define  CSV_TP_SELFCAP_RAW_RX_RANGE "SelfCapRawRxRange"
#define  CSV_TP_SELFCAP_NOIZE_TX_RANGE "SelfCapNoiseTxRange"   /*7p/f*/
#define  CSV_TP_SELFCAP_NOIZE_RX_RANGE "SelfCapNoiseRxRange"

#define WATCHDOG_TIMEOUT_S 2
#define FORCE_TIMEOUT_100MS 10
#define STATUS_WORK_INTERVAL 20	/* ms */
#define MAX_I2C_MSG_LENS 0x3F

#define RX_NUMBER 89		/*f01 control_base_addr + 57*/
#define TX_NUMBER 90		/*f01 control_base_addr + 58*/
#define RX_NUMBER_S3207 39	/*f11 control_base_addr + 39*/
#define TX_NUMBER_S3207 40	/*f11 control_base_addr + 40*/
#define FORCE_NUMER 9

#define BOOTLOADER_ID_OFFSET 0
#define BOOTLOADER_ID_OFFSET_V7 1

#define STATUS_IDLE 0
#define STATUS_BUSY 1

#define DATA_REPORT_INDEX_OFFSET 1
#define DATA_REPORT_DATA_OFFSET 3
#define CALIBRATION_INFO_OFFSET 0x0E

#define COMMAND_TIMEOUT_100MS 20
#define SENSOR_RX_MAPPING_OFFSET 1
#define SENSOR_TX_MAPPING_OFFSET 2

#define COMMAND_GET_REPORT 1
#define COMMAND_FORCE_CAL 2
#define COMMAND_FORCE_UPDATE 4
#define F54_READ_RATE_OFFSET 9

#define CONTROL_0_SIZE 1
#define CONTROL_1_SIZE 1
#define CONTROL_2_SIZE 2
#define CONTROL_3_SIZE 1
#define CONTROL_4_6_SIZE 3
#define CONTROL_7_SIZE 1
#define CONTROL_8_9_SIZE 3
#define CONTROL_10_SIZE 1
#define CONTROL_11_SIZE 2
#define CONTROL_12_13_SIZE 2
#define CONTROL_14_SIZE 1
#define CONTROL_15_SIZE 1
#define CONTROL_16_SIZE 1
#define CONTROL_17_SIZE 1
#define CONTROL_18_SIZE 1
#define CONTROL_19_SIZE 1
#define CONTROL_20_SIZE 1
#define CONTROL_21_SIZE 2
#define CONTROL_22_26_SIZE 7
#define CONTROL_27_SIZE 1
#define CONTROL_28_SIZE 2
#define CONTROL_29_SIZE 1
#define CONTROL_30_SIZE 1
#define CONTROL_31_SIZE 1
#define CONTROL_32_35_SIZE 8
#define CONTROL_36_SIZE 1
#define CONTROL_37_SIZE 1
#define CONTROL_38_SIZE 1
#define CONTROL_39_SIZE 1
#define CONTROL_40_SIZE 1
#define CONTROL_41_SIZE 1
#define CONTROL_42_SIZE 2
#define CONTROL_43_54_SIZE 13
#define CONTROL_55_56_SIZE 2
#define CONTROL_57_SIZE 1
#define CONTROL_58_SIZE 1
#define CONTROL_59_SIZE 2
#define CONTROL_60_62_SIZE 3
#define CONTROL_63_SIZE 1
#define CONTROL_64_67_SIZE 4
#define CONTROL_68_73_SIZE 8
#define CONTROL_74_SIZE 2
#define CONTROL_75_SIZE 1
#define CONTROL_76_SIZE 1
#define CONTROL_77_78_SIZE 2
#define CONTROL_79_83_SIZE 5
#define CONTROL_84_85_SIZE 2
#define CONTROL_86_SIZE 1
#define CONTROL_87_SIZE 1
#define CONTROL_88_SIZE 1
#define CONTROL_89_SIZE 1
#define CONTROL_90_SIZE 1
#define CONTROL_91_SIZE 1
#define CONTROL_92_SIZE 1
#define CONTROL_93_SIZE 1
#define CONTROL_94_SIZE 1
#define CONTROL_95_SIZE 1
#define CONTROL_96_SIZE 1
#define CONTROL_97_SIZE 1
#define CONTROL_98_SIZE 1
#define CONTROL_99_SIZE 1
#define CONTROL_100_SIZE 1
#define CONTROL_101_SIZE 1
#define CONTROL_102_SIZE 1
#define CONTROL_103_SIZE 1
#define CONTROL_104_SIZE 1
#define CONTROL_105_SIZE 1
#define CONTROL_106_SIZE 1
#define CONTROL_107_SIZE 1
#define CONTROL_108_SIZE 1
#define CONTROL_109_SIZE 1
#define CONTROL_110_SIZE 1
#define CONTROL_111_SIZE 1
#define CONTROL_112_SIZE 1
#define CONTROL_113_SIZE 1
#define CONTROL_114_SIZE 1
#define CONTROL_115_SIZE 1
#define CONTROL_116_SIZE 1
#define CONTROL_117_SIZE 1
#define CONTROL_118_SIZE 1
#define CONTROL_119_SIZE 1
#define CONTROL_120_SIZE 1
#define CONTROL_121_SIZE 1
#define CONTROL_122_SIZE 1
#define CONTROL_123_SIZE 1
#define CONTROL_124_SIZE 1
#define CONTROL_125_SIZE 1
#define CONTROL_126_SIZE 1
#define CONTROL_127_SIZE 1
#define CONTROL_128_SIZE 1
#define CONTROL_129_SIZE 1
#define CONTROL_130_SIZE 1
#define CONTROL_131_SIZE 1
#define CONTROL_132_SIZE 1
#define CONTROL_133_SIZE 1
#define CONTROL_134_SIZE 1
#define CONTROL_135_SIZE 1
#define CONTROL_136_SIZE 1
#define CONTROL_137_SIZE 1
#define CONTROL_138_SIZE 1
#define CONTROL_139_SIZE 1
#define CONTROL_140_SIZE 1
#define CONTROL_141_SIZE 1
#define CONTROL_142_SIZE 1
#define CONTROL_143_SIZE 1
#define CONTROL_144_SIZE 1
#define CONTROL_145_SIZE 1
#define CONTROL_146_SIZE 1
#define CONTROL_147_SIZE 1
#define CONTROL_148_SIZE 1
#define CONTROL_149_SIZE 1
#define CONTROL_163_SIZE 1
#define CONTROL_165_SIZE 1
#define CONTROL_166_SIZE 1
#define CONTROL_167_SIZE 1
#define CONTROL_176_SIZE 1
#define CONTROL_179_SIZE 1
#define CONTROL_188_SIZE 1

#define HIGH_RESISTANCE_DATA_SIZE 6
#define FULL_RAW_CAP_MIN_MAX_DATA_SIZE 4
#define TREX_DATA_SIZE 7

#define NO_AUTO_CAL_MASK 0x01
#define F54_BUF_LEN 80
#define TP_TEST_FAILED_REASON_LEN 20

#define TEST_PASS 1
#define TEST_FAIL 0
#define EE_SHORT_TEST_PASS 0
#define EE_SHORT_TEST_FAIL 1
#define SHORT_TEST_PASS "-5P"
#define SHORT_TEST_FAIL "-5F"
#define F54_TD43XX_EE_SHORT_SIZE 4
#define TD43XX_EE_SHORT_TEST_LEFT_SIZE 0x9
#define TD43XX_EE_SHORT_TEST_RIGHT_SIZE 0x9
#define SHIFT_ONE_BYTE 8
#define NORMAL_NUM_ONE 1
#define NORMAL_NUM_TWO 2
#define NORMAL_NUM_ONE_HUNDRED 100
#define HYBRID_BUF_SIZE 100
#define NOISE_DATA_LIMIT_SIZE 2
#define CSVFILE_NOISE_ROW 1
#define CSVFILE_NOISE_COLUMN 2
#define EE_SHORT_DATA_LIMIT_SIZE 2
#define CSVFILE_EE_SHORT_ROW 1
#define CSVFILE_EE_SHORT_COLUMN 2
#define LIMIT_ONE 0
#define LIMIT_TWO 1
#define CSV_PRODUCT_SYSTEM 1
#define CSV_ODM_SYSTEM 2
#define TEST_THRESHOLD_FAIL 1
int test_failed_reason[RAW_DATA_END] = {0};
static char tp_test_failed_reason[TP_TEST_FAILED_REASON_LEN] = { "-software_reason" };
static unsigned short report_rate_offset = 38;
static char buf_f54test_result[F54_BUF_LEN] = { 0 };	/*store mmi test result*/
static int *tx_delta_buf = NULL;
static int *rx_delta_buf = NULL;
static signed int *g_td43xx_rt95_part_one = NULL;
static signed int *g_td43xx_rt95_part_two = NULL;
extern char synaptics_raw_data_limit_flag;
static int mmi_buf_size;
static int mmi_hybrid_abs_delta = 0;
static int rawdata_size;
extern struct dsm_client *ts_dclient;
extern struct synaptics_rmi4_data *rmi4_data;
static int synaptics_rmi4_f54_attention(void);
static int synaptics_get_threshold_from_csvfile(int columns, int rows, char* target_name, int32_t *data);

enum f54_ctr_work_rate {
	F54_AUTO_RATE = 0,
	F54_60LOW_RATE = 2,
	F54_120HI_RATE = 4,
};

enum f54_data_work_rate {
	F54_DATA120_RATE = 8,
	F54_DATA60_RATE = 4,
};

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
	F54_HYBRID_ABS_DELTA_CAP = 59,
	F54_HYBRID_SENSING_RAW_CAP = 63,
	F54_CALIBRATION = 84,
	F54_TD43XX_EE_SHORT = 95,
	F54_TEST_100_TYPE = 100,
	F54_EXTEND_TRX_SHORT_INCELL = 126,

	INVALID_REPORT_TYPE = -1,
};

enum f54_rawdata_limit {
	RAW_DATA_UP = 0,
	RAW_DATA_LOW = 1,
	DELT_DATA_UP = 2,
	DELT_DATA_LOW = 3,
	RX_TO_RX_UP = 4,
	RX_TO_RX_LOW = 5,
	TX_TO_TX_UP = 6,
	TX_TO_TX_LOW = 7,
};

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
		} __packed;
		unsigned char data[14];
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

struct f54_control_86 {
	union {
		struct {
			unsigned char enable_high_noise_state:1;
			unsigned char dynamic_sense_display_ratio:2;
			unsigned char f54_ctrl86_b3__7:5;
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
struct f54_control_96 {
	unsigned short address;
};

struct f54_control_110 {
	union {
		struct {
			unsigned char active_stylus_rx_feedback_cap;
			unsigned char active_stylus_rx_feedback_cap_reference;
			unsigned char active_stylus_low_reference;
			unsigned char active_stylus_high_reference;
			unsigned char active_stylus_gain_control;
			unsigned char active_stylus_gain_control_reference;
			unsigned char active_stylus_timing_mode;
			unsigned char active_stylus_discovery_bursts;
			unsigned char active_stylus_detection_bursts;
			unsigned char active_stylus_discovery_noise_multiplier;
			unsigned char active_stylus_detection_envelope_min;
			unsigned char active_stylus_detection_envelope_max;
			unsigned char active_stylus_lose_count;
		} __packed;
		struct {
			unsigned char data[13];
			unsigned short address;
		} __packed;
	};
};

struct f54_control_149 {
	union {
		struct {
			unsigned char trans_cbc_global_cap_enable:1;
			unsigned char f54_ctrl149_b1__7:7;
		} __packed;
		struct {
			unsigned char data[1];
			unsigned short address;
		} __packed;
	};
};

struct f54_control_188 {
	union {
		struct {
			unsigned char start_calibration:1;
			unsigned char start_is_calibration:1;
			unsigned char frequency:2;
			unsigned char start_production_test:1;
			unsigned char short_test_calibration:1;
			unsigned char f54_ctrl188_b7:1;
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
	struct f54_control_86 *reg_86;
	struct f54_control_88 *reg_88;
	struct f54_control_96 *reg_96;
	struct f54_control_110 *reg_110;
	struct f54_control_149 *reg_149;
	struct f54_control_188 *reg_188;
};

struct f54_query_13 {
	union {
		struct {
			unsigned char has_ctrl86:1;
			unsigned char has_ctrl87:1;
			unsigned char has_ctrl87_sub0:1;
			unsigned char has_ctrl87_sub1:1;
			unsigned char has_ctrl87_sub2:1;
			unsigned char has_cidim:1;
			unsigned char has_noise_mitigation_enhancement:1;
			unsigned char has_rail_im:1;
		} __packed;
		unsigned char data[1];
	};
};

struct f54_query_15 {
	union {
		struct {
			unsigned char has_ctrl90:1;
			unsigned char has_transmit_strength:1;
			unsigned char has_ctrl87_sub3:1;
			unsigned char has_query16:1;
			unsigned char has_query20:1;
			unsigned char has_query21:1;
			unsigned char has_query22:1;
			unsigned char has_query25:1;
		} __packed;
		unsigned char data[1];
	};
};

struct f54_query_16 {
	union {
		struct {
			unsigned char has_query17:1;
			unsigned char has_data17:1;
			unsigned char has_ctrl92:1;
			unsigned char has_ctrl93:1;
			unsigned char has_ctrl94_query18:1;
			unsigned char has_ctrl95_query19:1;
			unsigned char has_ctrl99:1;
			unsigned char has_ctrl100:1;
		} __packed;
		unsigned char data[1];
	};
};

struct f54_query_21 {
	union {
		struct {
			unsigned char has_abs_rx:1;
			unsigned char has_abs_tx:1;
			unsigned char has_ctrl91:1;
			unsigned char has_ctrl96:1;
			unsigned char has_ctrl97:1;
			unsigned char has_ctrl98:1;
			unsigned char has_data19:1;
			unsigned char has_query24_data18:1;
		} __packed;
		unsigned char data[1];
	};
};
struct f54_query_22 {
	union {
		struct {
			unsigned char has_packed_image:1;
			unsigned char has_ctrl101:1;
			unsigned char has_dynamic_sense_display_ratio:1;
			unsigned char has_query23:1;
			unsigned char has_ctrl103_query26:1;
			unsigned char has_ctrl104:1;
			unsigned char has_ctrl105:1;
			unsigned char has_query28:1;
		} __packed;
		unsigned char data[1];
	};
};

struct f54_query_23 {
	union {
		struct {
			unsigned char has_ctrl102:1;
			unsigned char has_ctrl102_sub1:1;
			unsigned char has_ctrl102_sub2:1;
			unsigned char has_ctrl102_sub4:1;
			unsigned char has_ctrl102_sub5:1;
			unsigned char has_ctrl102_sub9:1;
			unsigned char has_ctrl102_sub10:1;
			unsigned char has_ctrl102_sub11:1;
		} __packed;
		unsigned char data[1];
	};
};

struct f54_query_25 {
	union {
		struct {
			unsigned char has_ctrl106:1;
			unsigned char has_ctrl102_sub12:1;
			unsigned char has_ctrl107:1;
			unsigned char has_ctrl108:1;
			unsigned char has_ctrl109:1;
			unsigned char has_data20:1;
			unsigned char f54_query25_b6:1;
			unsigned char has_query27:1;
		} __packed;
		unsigned char data[1];
	};
};

struct f54_query_27 {
	union {
		struct {
			unsigned char has_ctrl110:1;
			unsigned char has_data21:1;
			unsigned char has_ctrl111:1;
			unsigned char has_ctrl112:1;
			unsigned char has_ctrl113:1;
			unsigned char has_data22:1;
			unsigned char has_ctrl114:1;
			unsigned char has_query29:1;
		} __packed;
		unsigned char data[1];
	};
};

struct f54_query_29 {
	union {
		struct {
			unsigned char has_ctrl115:1;
			unsigned char has_ground_ring_options:1;
			unsigned char has_lost_bursts_tuning:1;
			unsigned char has_aux_exvcom2_select:1;
			unsigned char has_ctrl116:1;
			unsigned char has_data23:1;
			unsigned char has_ctrl117:1;
			unsigned char has_query30:1;
		} __packed;
		unsigned char data[1];
	};
};

struct f54_query_30 {
	union {
		struct {
			unsigned char has_ctrl118:1;
			unsigned char has_ctrl119:1;
			unsigned char has_ctrl120:1;
			unsigned char has_ctrl121:1;
			unsigned char has_ctrl122_query31:1;
			unsigned char has_ctrl123:1;
			unsigned char f54_query30_b6:1;
			unsigned char has_query32:1;
		} __packed;
		unsigned char data[1];
	};
};

struct f54_query_32 {
	union {
		struct {
			unsigned char has_ctrl125:1;
			unsigned char has_ctrl126:1;
			unsigned char has_ctrl127:1;
			unsigned char has_abs_charge_pump_disable:1;
			unsigned char has_query33:1;
			unsigned char has_data24:1;
			unsigned char has_query34:1;
			unsigned char has_query35:1;
		} __packed;
		unsigned char data[1];
	};
};

struct f54_query_33 {
	union {
		struct {
			unsigned char f54_query33_b0:1;
			unsigned char f54_query33_b1:1;
			unsigned char f54_query33_b2:1;
			unsigned char f54_query33_b3:1;
			unsigned char has_ctrl132:1;
			unsigned char has_ctrl133:1;
			unsigned char has_ctrl134:1;
			unsigned char has_query36:1;
		} __packed;
		unsigned char data[1];
	};
};

struct f54_query_35 {
	union {
		struct {
			unsigned char has_data25:1;
			unsigned char f54_query35_b1:1;
			unsigned char f54_query35_b2:1;
			unsigned char has_ctrl137:1;
			unsigned char has_ctrl138:1;
			unsigned char has_ctrl139:1;
			unsigned char has_data26:1;
			unsigned char has_ctrl140:1;
		} __packed;
		unsigned char data[1];
	};
};

struct f54_query_36 {
	union {
		struct {
			unsigned char f54_query36_b0:1;
			unsigned char has_ctrl142:1;
			unsigned char has_query37:1;
			unsigned char has_ctrl143:1;
			unsigned char has_ctrl144:1;
			unsigned char has_ctrl145:1;
			unsigned char has_ctrl146:1;
			unsigned char has_query38:1;
		} __packed;
		unsigned char data[1];
	};
};

struct f54_query_38 {
	union {
		struct {
			unsigned char has_ctrl147:1;
			unsigned char has_ctrl148:1;
			unsigned char has_ctrl149:1;
			unsigned char f54_query38_b3__6:4;
			unsigned char has_query39:1;
		} __packed;
		unsigned char data[1];
	};
};

struct f54_query_39 {
	union {
		struct {
			unsigned char f54_query39_b0__6:7;
			unsigned char has_query40:1;
		} __packed;
		unsigned char data[1];
	};
};

struct f54_query_40 {
	union {
		struct {
			unsigned char f54_query40_b0:1;
			unsigned char has_ctrl163_query41:1;
			unsigned char f54_query40_b2:1;
			unsigned char has_ctrl165_query42:1;
			unsigned char has_ctrl166:1;
			unsigned char has_ctrl167:1;
			unsigned char f54_query40_b6:1;
			unsigned char has_query43:1;
		} __packed;
		unsigned char data[1];
	};
};

struct f54_query_43 {
	union {
		struct {
			unsigned char f54_query43_b0__6:7;
			unsigned char has_query46:1;
		} __packed;
		unsigned char data[1];
	};
};

struct f54_query_46 {
	union {
		struct {
			unsigned char has_ctrl176:1;
			unsigned char f54_query46_b1:1;
			unsigned char has_ctrl179:1;
			unsigned char f54_query46_b3:1;
			unsigned char has_data27:1;
			unsigned char has_data28:1;
			unsigned char f54_query46_b6:1;
			unsigned char has_query47:1;
		} __packed;
		unsigned char data[1];
	};
};

struct f54_query_47 {
	union {
		struct {
			unsigned char f54_query47_b0__6:7;
			unsigned char has_query49:1;
		} __packed;
		unsigned char data[1];
	};
};

struct f54_query_49 {
	union {
		struct {
			unsigned char f54_query49_b0__1:2;
			unsigned char has_ctrl188:1;
			unsigned char has_data31:1;
			unsigned char f54_query49_b4__6:3;
			unsigned char has_query50:1;
		} __packed;
		unsigned char data[1];
	};
};

struct f54_query_50 {
	union {
		struct {
			unsigned char f54_query50_b0__6:7;
			unsigned char has_query51:1;
		} __packed;
		unsigned char data[1];
	};
};

struct f54_query_51 {
	union {
		struct {
			unsigned char f54_query51_b0__4:5;
			unsigned char has_query53_query54_ctrl198:1;
			unsigned char f54_query51_b6__7:2;
		} __packed;
		unsigned char data[1];
	};
};

struct f54_data_31 {
	union {
		struct {
			unsigned char is_calibration_crc:1;
			unsigned char calibration_crc:1;
			unsigned char short_test_row_number:5;
		} __packed;
		struct {
			unsigned char data[1];
			unsigned short address;
		} __packed;
	};
};

struct synaptics_rmi4_fn55_desc {
	unsigned short query_base_addr;
	unsigned short control_base_addr;
};
struct synaptics_fn54_statics_data {
	short RawimageAverage;
	short RawimageMaxNum;
	short RawimageMinNum;
	short RawimageNULL;
};
enum bl_version {
	V5 = 5,
	V6 = 6,
	V7 = 7,
};

struct synaptics_rmi4_f54_handle {
	bool no_auto_cal;
	bool skip_preparation;
	unsigned char tx_assigned;
	unsigned char rx_assigned;
	unsigned char status;
	unsigned char intr_mask;
	unsigned char intr_reg_num;
	unsigned char *report_data;
	unsigned char bootloader_id[2];
	unsigned short query_base_addr;
	unsigned short control_base_addr;
	unsigned short data_base_addr;
	unsigned short command_base_addr;
	unsigned short fifoindex;
	unsigned int report_size;
	unsigned int data_buffer_size;

	enum bl_version bl_version;
	enum f54_report_types report_type;

	char *mmi_buf;
	int *rawdatabuf;
	u32 *hybridbuf;
	struct f54_query query;
	struct f54_query_13 query_13;
	struct f54_query_15 query_15;
	struct f54_query_16 query_16;
	struct f54_query_21 query_21;
	struct f54_query_22 query_22;
	struct f54_query_23 query_23;
	struct f54_query_25 query_25;
	struct f54_query_27 query_27;
	struct f54_query_29 query_29;
	struct f54_query_30 query_30;
	struct f54_query_32 query_32;
	struct f54_query_33 query_33;
	struct f54_query_35 query_35;
	struct f54_query_36 query_36;
	struct f54_query_38 query_38;
	struct f54_query_39 query_39;
	struct f54_query_40 query_40;
	struct f54_query_43 query_43;
	struct f54_query_46 query_46;
	struct f54_query_47 query_47;
	struct f54_query_49 query_49;
	struct f54_query_50 query_50;
	struct f54_query_51 query_51;
	struct f54_data_31 data_31;
	struct f54_control control;
	struct kobject *attr_dir;
	struct synaptics_fn54_statics_data raw_statics_data;
	struct synaptics_fn54_statics_data delta_statics_data;
	struct synaptics_rmi4_exp_fn_ptr *fn_ptr;
	struct synaptics_rmi4_data *rmi4_data;
	struct synaptics_rmi4_fn55_desc *fn55;
	struct synaptics_rmi4_fn_desc f34_fd;
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

struct f55_query_3 {
	union {
		struct {
			unsigned char has_ctrl8:1;
			unsigned char has_ctrl9:1;
			unsigned char has_oncell_pattern_support:1;
			unsigned char has_data0:1;
			unsigned char has_single_wide_pattern_support:1;
			unsigned char has_mirrored_tx_pattern_support:1;
			unsigned char has_discrete_pattern_support:1;
			unsigned char has_query9:1;
		} __packed;
		unsigned char data[1];
	};
};

struct f55_query_5 {
	union {
		struct {
			unsigned char has_corner_compensation:1;
			unsigned char has_ctrl12:1;
			unsigned char has_trx_configuration:1;
			unsigned char has_ctrl13:1;
			unsigned char f55_query5_b4:1;
			unsigned char has_ctrl14:1;
			unsigned char has_basis_function:1;
			unsigned char has_query17:1;
		} __packed;
		unsigned char data[1];
	};
};

struct f55_query_17 {
	union {
		struct {
			unsigned char f55_query17_b0:1;
			unsigned char has_ctrl16:1;
			unsigned char has_ctrl18_ctrl19:1;
			unsigned char has_ctrl17:1;
			unsigned char has_ctrl20:1;
			unsigned char has_ctrl21:1;
			unsigned char has_ctrl22:1;
			unsigned char has_query18:1;
		} __packed;
		unsigned char data[1];
	};
};

struct f55_query_18 {
	union {
		struct {
			unsigned char has_ctrl23:1;
			unsigned char has_ctrl24:1;
			unsigned char has_query19:1;
			unsigned char has_ctrl25:1;
			unsigned char has_ctrl26:1;
			unsigned char has_ctrl27_query20:1;
			unsigned char has_ctrl28_query21:1;
			unsigned char has_query22:1;
		} __packed;
		unsigned char data[1];
	};
};

struct f55_query_22 {
	union {
		struct {
			unsigned char has_ctrl29:1;
			unsigned char has_query23:1;
			unsigned char has_guard_disable:1;
			unsigned char has_ctrl30:1;
			unsigned char has_ctrl31:1;
			unsigned char has_ctrl32:1;
			unsigned char has_query24_through_query27:1;
			unsigned char has_query28:1;
		} __packed;
		unsigned char data[1];
	};
};

struct f55_query_23 {
	union {
		struct {
			unsigned char amp_sensor_enabled:1;
			unsigned char image_transposed:1;
			unsigned char first_column_at_left_side:1;
			unsigned char size_of_column2mux:5;
		} __packed;
		unsigned char data[1];
	};
};

struct f55_query_28 {
	union {
		struct {
			unsigned char f55_query28_b0__4:5;
			unsigned char has_ctrl37:1;
			unsigned char has_query29:1;
			unsigned char has_query30:1;
		} __packed;
		unsigned char data[1];
	};
};

struct f55_query_30 {
	union {
		struct {
			unsigned char has_ctrl38:1;
			unsigned char has_query31_query32:1;
			unsigned char has_ctrl39:1;
			unsigned char has_ctrl40:1;
			unsigned char has_ctrl41:1;
			unsigned char has_ctrl42:1;
			unsigned char has_ctrl43_ctrl44:1;
			unsigned char has_query33:1;
		} __packed;
		unsigned char data[1];
	};
};

struct f55_query_33 {
	union {
		struct {
			unsigned char has_extended_amp_pad:1;
			unsigned char has_extended_amp_btn:1;
			unsigned char has_ctrl45_ctrl46:1;
			unsigned char f55_query33_b3:1;
			unsigned char has_ctrl47_sub0_sub1:1;
			unsigned char f55_query33_b5__7:3;
		} __packed;
		unsigned char data[1];
	};
};

struct f55_control_43 {
	union {
		struct {
			unsigned char swap_sensor_side:1;
			unsigned char f55_ctrl43_b1__7:7;
			unsigned char afe_l_mux_size:4;
			unsigned char afe_r_mux_size:4;
		} __packed;
		unsigned char data[2];
	};
};

struct synaptics_rmi4_f55_handle {
	bool amp_sensor;
	bool extended_amp;
	bool has_force;
	unsigned char size_of_column2mux;
	unsigned char afe_mux_offset;
	unsigned char force_tx_offset;
	unsigned char force_rx_offset;
	unsigned char *tx_assignment;
	unsigned char *rx_assignment;
	unsigned char *rx_physical;
	unsigned char *force_tx_assignment;
	unsigned char *force_rx_assignment;
	unsigned short query_base_addr;
	unsigned short control_base_addr;
	unsigned short data_base_addr;
	unsigned short command_base_addr;
	struct f55_query query;
	struct f55_query_3 query_3;
	struct f55_query_5 query_5;
	struct f55_query_17 query_17;
	struct f55_query_18 query_18;
	struct f55_query_22 query_22;
	struct f55_query_23 query_23;
	struct f55_query_28 query_28;
	struct f55_query_30 query_30;
	struct f55_query_33 query_33;
};
static struct synaptics_rmi4_f54_handle *f54;
static struct synaptics_rmi4_f55_handle *f55;

DECLARE_COMPLETION(synaptics_f54_remove_complete);

static void set_report_size(void)
{
	int rc = 0;
	switch (f54->report_type) {
	case F54_8BIT_IMAGE:
		f54->report_size =
		    f54->rmi4_data->num_of_rx * f54->rmi4_data->num_of_tx;
		break;
	case F54_16BIT_IMAGE:
	case F54_RAW_16BIT_IMAGE:
	case F54_TRUE_BASELINE:
	case F54_FULL_RAW_CAP:
	case F54_FULL_RAW_CAP_RX_COUPLING_COMP:
	case F54_SENSOR_SPEED:
	case F54_TEST_100_TYPE:
		f54->report_size =
		    2 * f54->rmi4_data->num_of_rx * f54->rmi4_data->num_of_tx;
		break;
	case F54_HIGH_RESISTANCE:
		f54->report_size = HIGH_RESISTANCE_DATA_SIZE;
		break;
	case F54_TX_TO_TX_SHORT:
	case F54_TX_OPEN:
		f54->report_size = (f54->rmi4_data->num_of_tx + 7) / 8;
		break;
	case F54_TX_TO_GROUND:
		f54->report_size = 3;
		break;
	case F54_RX_TO_RX1:
		/*edw*/
		if (f54->rmi4_data->num_of_rx < f54->rmi4_data->num_of_tx)
			f54->report_size =
			    2 * f54->rmi4_data->num_of_rx *
			    f54->rmi4_data->num_of_rx;
		else
			f54->report_size =
			    2 * f54->rmi4_data->num_of_rx *
			    f54->rmi4_data->num_of_tx;
		break;
		/*edw*/
	case F54_RX_OPENS1:
		if (f54->rmi4_data->num_of_rx < f54->rmi4_data->num_of_tx)
			f54->report_size =
			    2 * f54->rmi4_data->num_of_rx *
			    f54->rmi4_data->num_of_rx;
		else
			f54->report_size =
			    2 * f54->rmi4_data->num_of_rx *
			    f54->rmi4_data->num_of_tx;
		break;
	case F54_FULL_RAW_CAP_MIN_MAX:
		f54->report_size = FULL_RAW_CAP_MIN_MAX_DATA_SIZE;
		break;
	case F54_RX_TO_RX2:
	case F54_RX_OPENS2:
		if (f54->rmi4_data->num_of_rx <= f54->rmi4_data->num_of_tx)
			f54->report_size = 0;
		else
			f54->report_size =
			    2 * f54->rmi4_data->num_of_rx *
			    (f54->rmi4_data->num_of_rx -
			     f54->rmi4_data->num_of_tx);
		break;
	case F54_ADC_RANGE:
		if (f54->query.has_signal_clarity) {
			rc = f54->fn_ptr->read(f54->rmi4_data,
					       f54->control.reg_41->address,
					       f54->control.reg_41->data,
					       sizeof(f54->control.reg_41->
						      data));
			if (rc < 0) {
				TS_LOG_INFO("Failed to read control reg_41\n");
				f54->report_size = 0;
				break;
			}
			if (!f54->control.reg_41->no_signal_clarity) {
				if (f54->rmi4_data->num_of_tx % 4)
					f54->rmi4_data->num_of_tx +=
					    4 - (f54->rmi4_data->num_of_tx % 4);
			}
		}
		f54->report_size =
		    2 * f54->rmi4_data->num_of_rx * f54->rmi4_data->num_of_tx;
		break;
	case F54_TREX_OPENS:
	case F54_TREX_TO_GND:
	case F54_TREX_SHORTS:
		f54->report_size = TREX_DATA_SIZE;
		break;
	case F54_CALIBRATION:
		f54->report_size = CALIBRATION_DATA_SIZE;
		break;
	case F54_HYBRID_ABS_DELTA_CAP:
	case F54_HYBRID_SENSING_RAW_CAP:
		f54->report_size = 4 * (f54->rmi4_data->num_of_rx + f54->rmi4_data->num_of_tx);
		break;
	case F54_TD43XX_EE_SHORT:
		f54->report_size = F54_TD43XX_EE_SHORT_SIZE
			* f54->rmi4_data->num_of_tx
			* f54->rmi4_data->num_of_rx;
		break;
	default:
		f54->report_size = 0;
	}

	return;
}

/* when the report type is  3 or 9, we call this function to  to find open
* transmitter electrodes, open receiver electrodes, transmitter-to-ground
* shorts, receiver-to-ground shorts, and transmitter-to-receiver shorts. */
static int f54_rawimage_report(void)
{
	short Rawimage;
	short Result = 0;
	int rows_size = 0;
	int columns_size = 0;
	char file_path[100] = {0};
	char file_name[64] = {0};
	int j;
	int i;
	int raw_cap_uplimit =
	    f54->rmi4_data->synaptics_chip_data->raw_limit_buf[RAW_DATA_UP];
	int raw_cap_lowlimit =
	    f54->rmi4_data->synaptics_chip_data->raw_limit_buf[RAW_DATA_LOW];
	int RawimageSum = 0;
	short RawimageAverage = 0;
	short RawimageMaxNum, RawimageMinNum;
	struct ts_rawdata_limit_tab limit_tab = {0};
	int *rawdata_from_chip = NULL;

	TS_LOG_INFO
	    ("f54_rawimage_report: rx = %d, tx = %d, mmibuf_size =%d, raw_cap_uplimit = %d,raw_cap_lowlimit = %d\n",
	     f54->rmi4_data->num_of_rx, f54->rmi4_data->num_of_tx, mmi_buf_size,
	     raw_cap_uplimit, raw_cap_lowlimit);

	RawimageMaxNum = (f54->mmi_buf[0]) | (f54->mmi_buf[1] << 8);
	RawimageMinNum = (f54->mmi_buf[0]) | (f54->mmi_buf[1] << 8);
	for (i = 0; i < mmi_buf_size; i += 2) {
		Rawimage = (f54->mmi_buf[i]) | (f54->mmi_buf[i + 1] << 8);
		RawimageSum += Rawimage;
		if (RawimageMaxNum < Rawimage)
			RawimageMaxNum = Rawimage;
		if (RawimageMinNum > Rawimage)
			RawimageMinNum = Rawimage;
	}
	RawimageAverage = RawimageSum / (mmi_buf_size / 2);
	f54->raw_statics_data.RawimageAverage = RawimageAverage;
	f54->raw_statics_data.RawimageMaxNum = RawimageMaxNum;
	f54->raw_statics_data.RawimageMinNum = RawimageMinNum;

	TS_LOG_INFO("raw_test_type:%d\n", f54->rmi4_data->synaptics_chip_data->raw_test_type);
	if (f54->rmi4_data->synaptics_chip_data->raw_test_type == RAW_DATA_TEST_TYPE_SINGLE_POINT) {

		rows_size = f54->rmi4_data->num_of_rx;
		columns_size = f54->rmi4_data->num_of_tx;

		TS_LOG_INFO("rows:%d, columns:%d\n", rows_size, columns_size);

		limit_tab.MutualRawMax =
			(int32_t*)kzalloc((rows_size+1)*(columns_size+1)*sizeof(int32_t), GFP_KERNEL);
		limit_tab.MutualRawMin =
			(int32_t*)kzalloc((rows_size+1)*(columns_size+1)*sizeof(int32_t), GFP_KERNEL);
		rawdata_from_chip = (int*)kzalloc((rows_size * columns_size)*sizeof(int), GFP_KERNEL);
		if (!limit_tab.MutualRawMax || !limit_tab.MutualRawMin || !rawdata_from_chip){
			TS_LOG_ERR("kzalloc error: MutualRawMax:%p, MutualRawMin:%p, rawdata_from_chip:%p\n",
				limit_tab.MutualRawMax, limit_tab.MutualRawMin, rawdata_from_chip);
			goto error_release_mem;
		}
		if (!strnlen(f54->rmi4_data->synaptics_chip_data->ts_platform_data->product_name, MAX_STR_LEN-1)
			|| !strnlen(f54->rmi4_data->synaptics_chip_data->chip_name, MAX_STR_LEN-1)
			|| !strnlen(f54->rmi4_data->rmi4_mod_info.project_id_string, SYNAPTICS_RMI4_PROJECT_ID_SIZE)
			|| f54->rmi4_data->module_name == NULL) {
			TS_LOG_INFO("Threshold file name is not detected\n");
			goto error_release_mem;
		}
		snprintf(file_name, sizeof(file_name) - 1, "%s_%s_%s_%s_raw.csv",
			f54->rmi4_data->synaptics_chip_data->ts_platform_data->product_name,
			f54->rmi4_data->synaptics_chip_data->chip_name,
			f54->rmi4_data->rmi4_mod_info.project_id_string,
			f54->rmi4_data->module_name);
#ifdef BOARD_VENDORIMAGE_FILE_SYSTEM_TYPE
		snprintf(file_path, sizeof(file_path) -1, "/product/etc/firmware/ts/%s", file_name);
#else
		snprintf(file_path, sizeof(file_path) -1, "/odm/etc/firmware/ts/%s", file_name);
#endif
		TS_LOG_INFO("threshold file name:%s, rows_size=%d, columns_size=%d\n", file_path, rows_size, columns_size);

		Result   =ts_kit_parse_csvfile(file_path, CSV_TP_CAP_RAW_MAX, limit_tab.MutualRawMax, rows_size, columns_size);
		Result +=ts_kit_parse_csvfile(file_path, CSV_TP_CAP_RAW_MIN, limit_tab.MutualRawMin, rows_size, columns_size);
		if (0 != Result) {
			TS_LOG_ERR("csv file parse fail:%s\n", file_path);
			goto error_release_mem;
		} else {
			TS_LOG_INFO("rawdata compare start\n");
			for (i = 0, j = 0; i < mmi_buf_size; i += 2, j++) {
				rawdata_from_chip[j] = (f54->mmi_buf[i]) | (f54->mmi_buf[i + 1] << 8);
			}

			if (f54->rmi4_data->synaptics_chip_data->rawdata_arrange_type == TS_RAWDATA_TRANS_ABCD2CBAD
				|| f54->rmi4_data->synaptics_chip_data->rawdata_arrange_type == TS_RAWDATA_TRANS_ABCD2ADCB) {
				ts_kit_rotate_rawdata_abcd2cbad(rows_size, columns_size,
					 rawdata_from_chip, f54->rmi4_data->synaptics_chip_data->rawdata_arrange_type);
			}

			for (i = 0; i < rows_size; i++) {
				for (j = 0; j < columns_size; j++) {
					TS_LOG_DEBUG("\t%u", rawdata_from_chip[i*columns_size + j]);
				}
				TS_LOG_DEBUG("\n");
			}

			for (i = 0; i < (mmi_buf_size / 2); i++) {
				if (rawdata_from_chip[i] > limit_tab.MutualRawMin[i]
					&& rawdata_from_chip[i] < limit_tab.MutualRawMax[i]){
					Result++;
				}else{
					TS_LOG_ERR("error rawdata[%d]:%d out of range, min:%d, max:%d\n",
						i, rawdata_from_chip[i], limit_tab.MutualRawMin[i], limit_tab.MutualRawMax[i]);
				}
			}

			/* rawdata check done, release mem */
			if (limit_tab.MutualRawMax) {
				kfree(limit_tab.MutualRawMax);
				limit_tab.MutualRawMax = NULL;
			}
			if (limit_tab.MutualRawMin) {
				kfree(limit_tab.MutualRawMin);
				limit_tab.MutualRawMin = NULL;
			}
			if (rawdata_from_chip) {
				kfree(rawdata_from_chip);
				rawdata_from_chip = NULL;
			}
		}
	}else{
		for (i = 0; i < mmi_buf_size; i += 2) {
			Rawimage = (f54->mmi_buf[i]) | (f54->mmi_buf[i + 1] << 8);
			if (synaptics_raw_data_limit_flag) {
				raw_cap_uplimit = RawCapUpperLimit[i / 2];
				raw_cap_lowlimit = RawCapLowerLimit[i / 2];
			}
			if ((Rawimage >= raw_cap_lowlimit)
			    && (Rawimage <= raw_cap_uplimit)) {
				Result++;
			} else {
				TS_LOG_INFO("[%d,%d]\n", i / 2, Rawimage);
			}
		}
	}
	if (Result == (mmi_buf_size / 2)) {
		TS_LOG_DEBUG("rawdata is all right, Result = %d\n", Result);
		return 1;
	} else {
		TS_LOG_ERR("rawdata is out of range, Result = %d\n", Result);
		return 0;
	}
error_release_mem:
	if (limit_tab.MutualRawMax){
		kfree(limit_tab.MutualRawMax);
		limit_tab.MutualRawMax = NULL;
	}
	if (limit_tab.MutualRawMin){
		kfree(limit_tab.MutualRawMin);
		limit_tab.MutualRawMin = NULL;
	}
	if (rawdata_from_chip){
		kfree(rawdata_from_chip);
		rawdata_from_chip = NULL;
	}
	return 0;
}

static int test_wait_for_command_completion(void)
{
	int retval = 0;
	unsigned char value = 0;
	unsigned char timeout_count = 0;
	struct synaptics_rmi4_data *rmi4_data = f54->rmi4_data;

	timeout_count = 0;
	do {
		retval = f54->fn_ptr->read(rmi4_data,
					f54->command_base_addr,
					&value,
					sizeof(value));
		if (retval < 0) {
			dev_err(rmi4_data->pdev->dev.parent,
						"%s: Failed to read command register\n",
						__func__);
			return retval;
		}

		if ((value & 0x01) == 0x00)
		  break;

		msleep(100);
		timeout_count++;
	} while (timeout_count < COMMAND_TIMEOUT_100MS);

	if (timeout_count == COMMAND_TIMEOUT_100MS) {
		TS_LOG_ERR("%s: Timed out waiting for command completion\n",
					__func__);
		return -ETIMEDOUT;
	}

	return 0;
}

static int test_do_command(unsigned char command)
{
	int retval = 0;
	struct synaptics_rmi4_data *rmi4_data = f54->rmi4_data;

	retval = f54->fn_ptr->write(rmi4_data,
				f54->command_base_addr,
				&command,
				sizeof(command));
	if (retval < 0) {
		TS_LOG_ERR("%s: Failed to write command\n",
					__func__);
		return retval;
	}

	retval = test_wait_for_command_completion();
	if (retval < 0)
	  return retval;

	return 0;
}

static int write_to_f54_register(unsigned char report_type)
{
	unsigned char command;
	int result;
	unsigned char ready_report = 0xFF;
	int retry_times = 0;

	TS_LOG_INFO("report type = %d\n", report_type);

	command = report_type;
	/*set report type */
	if (F54_RX_TO_RX1 != command) {
		result =
		    f54->fn_ptr->write(f54->rmi4_data, f54->data_base_addr,
				       &command, 1);
		if (result < 0) {
			TS_LOG_ERR("Could not write report type to 0x%04x\n",
				   f54->data_base_addr);
			return result;
		}
	}
	mdelay(5);

	/*set get_report to 1 */
	command = (unsigned char)COMMAND_GET_REPORT;
	result =
	    f54->fn_ptr->write(f54->rmi4_data, f54->command_base_addr, &command,
			       1);

retry:
	msleep(100);
	result =
	    f54->fn_ptr->read(f54->rmi4_data, f54->command_base_addr,
			      &ready_report, 1);

	/*report is ready when COMMAND_GET_REPORT bit is clear */
	if ((ready_report & COMMAND_GET_REPORT) && retry_times < 20) {
		retry_times++;
	TS_LOG_INFO("report is not ready, retry times: %d, ready_report: %d\n",
		    retry_times + 1, ready_report);
		goto retry;
	}

	return result;
}

static int mmi_add_static_data(void)
{
	int i;

	i = strlen(buf_f54test_result);
	if (i >= F54_BUF_LEN) {
		return -EINVAL;
	}
	snprintf((buf_f54test_result + i), F54_BUF_LEN - i,
		 "[%4d,%4d,%4d]",
		 f54->raw_statics_data.RawimageAverage,
		 f54->raw_statics_data.RawimageMaxNum,
		 f54->raw_statics_data.RawimageMinNum);

	i = strlen(buf_f54test_result);
	if (i >= F54_BUF_LEN) {
		return -EINVAL;
	}
	snprintf((buf_f54test_result + i), F54_BUF_LEN - i,
		 "[%4d,%4d,%4d]",
		 f54->delta_statics_data.RawimageAverage,
		 f54->delta_statics_data.RawimageMaxNum,
		 f54->delta_statics_data.RawimageMinNum);

	return 0;
}

static int f54_deltarawimage_report(void)
{
	short Rawimage;
	int i;
	int delt_cap_uplimit =
	    f54->rmi4_data->synaptics_chip_data->raw_limit_buf[DELT_DATA_UP];
	int delt_cap_lowlimit =
	    f54->rmi4_data->synaptics_chip_data->raw_limit_buf[DELT_DATA_LOW];
	int DeltaRawimageSum = 0;
	short DeltaRawimageAverage = 0;
	short DeltaRawimageMaxNum, DeltaRawimageMinNum;
	short result = 0;
	struct ts_rawdata_limit_tab limit_tab = {0};
	int ret = 0;

	if (f54->rmi4_data->synaptics_chip_data->test_capacitance_via_csvfile) {
		limit_tab.unique_test = kzalloc(sizeof(struct ts_unique_capacitance_test), GFP_KERNEL);
		if (!limit_tab.unique_test) {
			TS_LOG_ERR("ts_unique_capacitance_test kzalloc error\n");
			ret = TEST_FAIL;
			goto error_release_mem;
		}
		limit_tab.unique_test->noise_data_limit =
				(int32_t*)kzalloc(NOISE_DATA_LIMIT_SIZE * sizeof(int32_t), GFP_KERNEL);
		if (!limit_tab.unique_test->noise_data_limit) {
			TS_LOG_ERR("noise_data_limit buffer kzalloc error\n");
			ret = TEST_FAIL;
			goto error_release_mem;
		}
		if (TEST_FAIL == synaptics_get_threshold_from_csvfile(CSVFILE_NOISE_COLUMN,
			CSVFILE_NOISE_ROW, CSV_TP_NOISE_DATA_LIMIT, limit_tab.unique_test->noise_data_limit)) {
			TS_LOG_ERR("csvfile get noise_data_limit err, use dts config.\n");
		} else {
			delt_cap_uplimit = limit_tab.unique_test->noise_data_limit[LIMIT_TWO];
			delt_cap_lowlimit = limit_tab.unique_test->noise_data_limit[LIMIT_ONE];
		}
	}

	TS_LOG_INFO
	    ("f54_deltarawimage_report enter, delt_cap_uplimit = %d, delt_cap_lowlimit = %d\n",
	     delt_cap_uplimit, delt_cap_lowlimit);

	DeltaRawimageMaxNum = (f54->mmi_buf[0]) | (f54->mmi_buf[1] << 8);
	DeltaRawimageMinNum = (f54->mmi_buf[0]) | (f54->mmi_buf[1] << 8);
	for (i = 0; i < mmi_buf_size; i += 2) {
		Rawimage = (f54->mmi_buf[i]) | (f54->mmi_buf[i + 1] << 8);
		DeltaRawimageSum += Rawimage;
		if (DeltaRawimageMaxNum < Rawimage)
			DeltaRawimageMaxNum = Rawimage;
		if (DeltaRawimageMinNum > Rawimage)
			DeltaRawimageMinNum = Rawimage;
	}
	DeltaRawimageAverage = DeltaRawimageSum / (mmi_buf_size / 2);
	f54->delta_statics_data.RawimageAverage = DeltaRawimageAverage;
	f54->delta_statics_data.RawimageMaxNum = DeltaRawimageMaxNum;
	f54->delta_statics_data.RawimageMinNum = DeltaRawimageMinNum;
	if (synaptics_raw_data_limit_flag) {
		delt_cap_lowlimit = NoiseCapLowerLimit;
		delt_cap_uplimit = NoiseCapUpperLimit;
	}
	for (i = 0; i < mmi_buf_size; i += 2) {
		Rawimage = (f54->mmi_buf[i]) | (f54->mmi_buf[i + 1] << 8);
		if ((Rawimage >= delt_cap_lowlimit)
		    && (Rawimage <= delt_cap_uplimit)) {
			result++;
		} else {
			TS_LOG_INFO("[%d,%d]\n", i / 2, Rawimage);
		}
	}

	if (result == (mmi_buf_size / 2)) {
		TS_LOG_DEBUG("deltadata is all right, Result = %d\n", result);
		ret = TEST_PASS;
	} else {
		TS_LOG_ERR("deltadata is out of range, Result = %d\n", result);
		ret = TEST_FAIL;
		test_failed_reason[RAW_DATA_TYPE_Noise] = TEST_THRESHOLD_FAIL;
	}
error_release_mem:
	if (limit_tab.unique_test && limit_tab.unique_test->noise_data_limit){
		kfree(limit_tab.unique_test->noise_data_limit);
		limit_tab.unique_test->noise_data_limit = NULL;
	}
	if (limit_tab.unique_test) {
		kfree(limit_tab.unique_test);
		limit_tab.unique_test = NULL;
	}
	return ret;
}

static void mmi_deltacapacitance_test(void)
{
	unsigned char command;
	int result = 0;
	command = (unsigned char)F54_16BIT_IMAGE;
	TS_LOG_INFO("mmi_deltacapacitance_test called\n");
	write_to_f54_register(command);
	f54->report_type = command;
	result = synaptics_rmi4_f54_attention();
	if (result < 0) {
		TS_LOG_ERR("Failed to get data\n");
		strncat(buf_f54test_result, "-3F", MAX_STR_LEN);
		return;
	}
	result = f54_deltarawimage_report();
	if (1 == result) {
		strncat(buf_f54test_result, "-3P", MAX_STR_LEN);
	} else {
		TS_LOG_ERR
		    ("deltadata test is out of range, test result is 3F\n");
		strncat(buf_f54test_result, "-3F", MAX_STR_LEN);
		strncpy(tp_test_failed_reason, "-panel_reason",
			TP_TEST_FAILED_REASON_LEN);
	}
	return;
}

static void put_hybrid_data(int index)
{
	int i, j;
	u32 temp;

	for (i = 0, j = index; i < mmi_hybrid_abs_delta; i += 4, j++) {
		temp = (f54->mmi_buf[i]) | (f54->mmi_buf[i + 1] << 8) | (f54->mmi_buf[i + 2] << 16) | (f54->mmi_buf[i + 3] << 24);
		f54->hybridbuf[j] = temp;
	}
	TS_LOG_INFO("*******put_hybrid_data start****mmi_hybrid_abs_delta=%d****\n", mmi_hybrid_abs_delta);
	for (i = index; i < ((mmi_hybrid_abs_delta /4) + index); i++) {
		TS_LOG_INFO(" %d\n", f54->hybridbuf[i]);
	}
	TS_LOG_INFO ("*******put_hybrid_data end********\n");
}

static bool is_in_range(int inputdata, int uplimit, int downlimit)
{
     if ((inputdata >= downlimit)&& (inputdata <= uplimit))
         return true;
     else
         return false;
}

static void check_hybrid_raw_cap(void)
{
	int i;
	int index = mmi_hybrid_abs_delta / 4;
	int result = 0;
	char file_path[100] = {0};
	char file_name[64] = {0};
	int32_t  hybird_raw_rx_range[HYBRID_BUF_SIZE]={0};
	int32_t  hybird_raw_tx_range[HYBRID_BUF_SIZE]={0};
       int rx_num = f54->rmi4_data->num_of_rx;
       int tx_num = f54->rmi4_data->num_of_tx;

	snprintf(file_name, sizeof(file_name) -1, "%s_%s_%s_%s_raw.csv",
			f54->rmi4_data->synaptics_chip_data->ts_platform_data->product_name,
			f54->rmi4_data->synaptics_chip_data->chip_name,
			f54->rmi4_data->rmi4_mod_info.project_id_string,
			f54->rmi4_data->module_name);
	#ifdef BOARD_VENDORIMAGE_FILE_SYSTEM_TYPE
	snprintf(file_path, sizeof(file_path) -1, "/product/etc/firmware/ts/%s", file_name);
	#else
	snprintf(file_path, sizeof(file_path) -1, "/odm/etc/firmware/ts/%s", file_name);
	#endif

	TS_LOG_INFO("%s(%d), threshold file name:%s \n", __func__, __LINE__, file_path);

	result = ts_kit_parse_csvfile(file_path, CSV_TP_SELFCAP_RAW_TX_RANGE,
		hybird_raw_tx_range, 2, tx_num);
	if(0 != result){
	     TS_LOG_ERR("%s(%d), ts_kit_parse_csvfile  parse tx range fail result=%dis 6F\n", __func__, __LINE__, result);
	     goto test_fail;
	}

	for(i = 0; i < tx_num; i++){
		if( false == is_in_range(f54->hybridbuf[index+i+rx_num],
			hybird_raw_tx_range[i+tx_num], hybird_raw_tx_range[i]) ) {
		    TS_LOG_ERR("%s(%d), check_hybrid_raw_cap test is out of range, test result is 6F\n", __func__, __LINE__);
		    strncpy(tp_test_failed_reason, "-panel_reason",TP_TEST_FAILED_REASON_LEN);
			test_failed_reason[RAW_DATA_TYPE_SelfCap] = TEST_THRESHOLD_FAIL;
		    goto test_fail;
		}
	}
	result = ts_kit_parse_csvfile(file_path, CSV_TP_SELFCAP_RAW_RX_RANGE,
		hybird_raw_rx_range, 2, rx_num);
	if(0 != result){
	     TS_LOG_ERR("%s(%d), ts_kit_parse_csvfile  parse rx fail result=%dis 6F\n", __func__, __LINE__, result);
	     goto test_fail;
	}

	for(i = 0; i < rx_num; i++){
		if(false == is_in_range(f54->hybridbuf[index+i],
			hybird_raw_rx_range[i+rx_num], hybird_raw_rx_range[i]) ){
		    TS_LOG_ERR("%s(%d), check_hybrid_raw_cap test is out of range, test result is 6F\n", __func__, __LINE__);
		    strncpy(tp_test_failed_reason, "-panel_reason",TP_TEST_FAILED_REASON_LEN);
			test_failed_reason[RAW_DATA_TYPE_SelfCap] = TEST_THRESHOLD_FAIL;
		    goto test_fail;
		}
	}

	TS_LOG_INFO("%s(%d), hybrid_abs_delta_report_size = %d. test result is 6P \n", __func__, __LINE__, f54->report_size);
	strncat(buf_f54test_result, "-6P", MAX_STR_LEN);
       return;

test_fail:
	strncat(buf_f54test_result, "-6F", MAX_STR_LEN);
	return;
}

static void check_hybrid_abs_delta(void)
{
	int i;
	int index = 0;
	char file_path[100] = {0};
	char file_name[64] = {0};
	int result =0;
	int32_t  hybird_abs_rx_range[HYBRID_BUF_SIZE]={0};
	int32_t  hybird_abs_tx_range[HYBRID_BUF_SIZE]={0};
       int rx_num = f54->rmi4_data->num_of_rx;
       int tx_num = f54->rmi4_data->num_of_tx;

	snprintf(file_name, sizeof(file_name), "%s_%s_%s_%s_raw.csv",
			f54->rmi4_data->synaptics_chip_data->ts_platform_data->product_name,
			f54->rmi4_data->synaptics_chip_data->chip_name,
			f54->rmi4_data->rmi4_mod_info.project_id_string,
			f54->rmi4_data->module_name);
	#ifdef BOARD_VENDORIMAGE_FILE_SYSTEM_TYPE
	snprintf(file_path, sizeof(file_path), "/product/etc/firmware/ts/%s", file_name);
	#else
	snprintf(file_path, sizeof(file_path), "/odm/etc/firmware/ts/%s", file_name);
	#endif

	TS_LOG_INFO("%s(%d), threshold file name:%s \n", __func__, __LINE__, file_path);

	result = ts_kit_parse_csvfile(file_path, CSV_TP_SELFCAP_NOIZE_TX_RANGE,
		hybird_abs_tx_range, 2, tx_num);
	if(0 != result){
	     TS_LOG_ERR("%s(%d), ts_kit_parse_csvfile  parse tx fail result=%dis 9F\n", __func__, __LINE__, result);
	     goto test_fail;
	}

	for(i = 0; i < tx_num; i++){
		if(false == is_in_range(f54->hybridbuf[index+i+rx_num],
			hybird_abs_tx_range[i+tx_num], hybird_abs_tx_range[i]) ){
		    TS_LOG_ERR("%s(%d), hybrid_abs_delta test is out of range, test result is 9F\n", __func__, __LINE__);
		    strncpy(tp_test_failed_reason, "-panel_reason",
			TP_TEST_FAILED_REASON_LEN);
			test_failed_reason[RAW_DATA_TYPE_SelfNoisetest] = TEST_THRESHOLD_FAIL;
		    goto test_fail;
		}
	}

	result = ts_kit_parse_csvfile(file_path, CSV_TP_SELFCAP_NOIZE_RX_RANGE,
		hybird_abs_rx_range, 2, rx_num);
	if(0 != result){
	     TS_LOG_ERR("%s(%d), ts_kit_parse_csvfile  parse rx fail result=%dis 9F\n", __func__, __LINE__, result);
	     goto test_fail;
	}

	for(i = 0; i < rx_num; i++){
		if(false == is_in_range(f54->hybridbuf[index+i],
			hybird_abs_rx_range[i+rx_num], hybird_abs_rx_range[i])){
		    TS_LOG_ERR("%s(%d), hybrid_abs_delta test is out of range, test result is 9F\n", __func__, __LINE__);
		    strncpy(tp_test_failed_reason, "-panel_reason",
			TP_TEST_FAILED_REASON_LEN);
			test_failed_reason[RAW_DATA_TYPE_SelfNoisetest] = TEST_THRESHOLD_FAIL;
		    goto test_fail;
		}
	}

	TS_LOG_INFO("%s(%d), hybrid_abs_delta_report_size = %d. test result is 9P \n",__func__, __LINE__, f54->report_size);
	strncat(buf_f54test_result, "-9P", MAX_STR_LEN);
       return;

test_fail:
	strncat(buf_f54test_result, "-9F", MAX_STR_LEN);
	return;
}

static int mmi_hybrid_abs_delta_test(void)
{
	unsigned char command = 0;
	int result = 0;
	command = (unsigned char)F54_HYBRID_ABS_DELTA_CAP;
	TS_LOG_INFO("%s(%d), mmi_hybrid_abs_delta_test called\n", __func__, __LINE__);
	write_to_f54_register(command);
	f54->report_type = command;
	result = synaptics_rmi4_f54_attention();
	if (result < 0) {
		TS_LOG_ERR("%s(%d), Failed to get data\n", __func__, __LINE__);
		strncat(buf_f54test_result, "-9F", MAX_STR_LEN);
	}
	else{
		put_hybrid_data(0);
		check_hybrid_abs_delta();
	}
	return result;
}

static int mmi_hybrid_raw_cap_test(void)
{
	unsigned char command;
	int result = 0;
	command = (unsigned char)F54_HYBRID_SENSING_RAW_CAP;
	TS_LOG_INFO("%s(%d), mmi_hybrid_raw_cap_test called\n", __func__, __LINE__);
	write_to_f54_register(command);
	f54->report_type = command;
	result = synaptics_rmi4_f54_attention();
	if (result < 0) {
		TS_LOG_ERR("%s(%d), Failed to get data\n", __func__, __LINE__);
		strncat(buf_f54test_result, "-6F", MAX_STR_LEN);
	}
	else{
		mmi_hybrid_abs_delta = f54->report_size;
	       put_hybrid_data(mmi_hybrid_abs_delta / 4);
	       check_hybrid_raw_cap();
	}
	return result;
}

static short FindMedian(short* pdata, int num)
{
	int i = 0;
	int j = 0;
	short temp = 0;
	short *value = NULL;
	short median = 0;

	value = (short *)kzalloc( num * sizeof(short), GFP_KERNEL);
	if (!value) {
		TS_LOG_ERR("%s(%d), failed to malloc\n", __func__, __LINE__);
		return -ENOMEM;
	}
	for(i = 0; i < num; i++) {
		*(value+i) = *(pdata+i);
	}

	/*sorting*/
	for (i = 1; i <= num-1; i++) {
		for (j = 1; j <= num-i; j++) {
			if (*(value+j-1) <= *(value+j)) {
				temp = *(value+j-1);
				*(value+j-1) = *(value+j);
				*(value+j) = temp;
			}
			else {
				continue;
			}
		}
	}
	/*calculation of median*/
	if (0 == num % NORMAL_NUM_TWO) {
		median = (*(value + (num/NORMAL_NUM_TWO - NORMAL_NUM_ONE)) +
			*(value + (num/NORMAL_NUM_TWO)))/NORMAL_NUM_TWO;
	}
	else {
		median = *(value+(num/NORMAL_NUM_TWO));
	}

	if(value) {
		kfree(value);
	}

	return median;
}

static int td43xx_ee_short_normalize_data(signed short * image)
{
	int retval = 0;
	int i = 0, j = 0;
	unsigned char tx_num = f54->rmi4_data->num_of_tx;
	unsigned char rx_num = f54->rmi4_data->num_of_rx;
	int part_two_limit = f54->rmi4_data->synaptics_chip_data->tddi_ee_short_test_parttwo_limit;
	unsigned char left_size = TD43XX_EE_SHORT_TEST_LEFT_SIZE; //0x09
	unsigned char right_size = TD43XX_EE_SHORT_TEST_RIGHT_SIZE;	//0x09
	signed short *report_data_16 = NULL;
	signed short *left_median = NULL;
	signed short *right_median = NULL;
	signed short *left_column_buf = NULL;
	signed short *right_column_buf = NULL;
	signed int temp = 0;
	char buf[MAX_CAP_DATA_SIZE_FOR_EESHORT] = {0};
	int tx_rx_delta_size = 0;
	int lens = 0;
	if (!image) {
		TS_LOG_ERR("%s(%d), td43xx_ee_short_data image is NULL\n", __func__, __LINE__);
		return -ENOMEM;
	}
	if(rx_num > 200){//rx's max number possibly
		TS_LOG_ERR("%s(%d), rx num is too large\n", __func__, __LINE__);
		return -EINVAL;
	}
	right_median = (unsigned short *) kzalloc(rx_num * sizeof(short), GFP_KERNEL);
	if (!right_median) {
		retval = -ENOMEM;
		TS_LOG_ERR("%s(%d), failed to malloc right_median\n", __func__, __LINE__);
		goto exit;
	}

	left_median = (unsigned short *) kzalloc(rx_num * sizeof(short), GFP_KERNEL);
	if (!left_median) {
		retval = -ENOMEM;
		TS_LOG_ERR("%s(%d), failed to malloc left_median\n", __func__, __LINE__);
		goto exit;
	}

	right_column_buf = (unsigned short *) kzalloc(right_size * rx_num * sizeof(short), GFP_KERNEL);
	if (!right_column_buf ) {
		retval = -ENOMEM;
		TS_LOG_ERR("%s(%d), failed to malloc right_column_buf\n", __func__, __LINE__);
		goto exit;
	}

	left_column_buf = (unsigned short *) kzalloc(left_size * rx_num * sizeof(short), GFP_KERNEL);
	if (!left_column_buf ) {
		retval = -ENOMEM;
		TS_LOG_ERR("%s(%d), failed to malloc left_column_buf\n", __func__, __LINE__);
		goto exit;
	}

	report_data_16 = image;
	for (i = 0; i < rx_num; i++) {
		for (j = 0; j < right_size; j++) {
			right_column_buf[i * right_size + j] =
					report_data_16[j * rx_num + i];
		}
	}

	report_data_16 = image + right_size * rx_num;
	for (i = 0; i < rx_num; i++) {
		for (j = 0; j < left_size; j++) {
			left_column_buf[i * left_size + j] =
					report_data_16[j * rx_num + i];
		}
	}

	for (i = 0; i < rx_num; i++) {
		right_median[i] = FindMedian(right_column_buf + i * right_size, right_size);
		left_median[i] = FindMedian(left_column_buf + i * left_size, left_size);
		if (-ENOMEM == right_median[i] || -ENOMEM == left_median[i]) {
			TS_LOG_ERR("failed to get midian[%d] value\n", i);
			retval = -ENOMEM;
			goto exit;
		}
		if (! right_median[i] || ! left_median[i]) {
			TS_LOG_ERR("the median value is 0.\n");
			retval = -EINVAL;
			goto exit;
		}
		TS_LOG_DEBUG("right_median[%d] = %d\n", i, right_median[i]);
		TS_LOG_DEBUG("left_median[%d] = %d\n", i, left_median[i]);
	}
	if (NULL != g_td43xx_rt95_part_two) {
		if (!tx_num || !rx_num) {
			retval = -ENOMEM;
			TS_LOG_ERR("%s(%d),tx_num=%d rx_num=%d\n", __func__, __LINE__,tx_num,rx_num);
			goto exit;
		} else {
			tx_rx_delta_size = (TX_RX_BUF_MAX<=(tx_num*rx_num*MAX_CAP_DATA_SIZE))?TX_RX_BUF_MAX:(tx_num*rx_num*MAX_CAP_DATA_SIZE);
		}
		snprintf(g_td43xx_rt95_part_two, PART_ONE_LIMIT_LENGTH, "%s\n", "part_two:");
		lens += PART_ONE_LIMIT_LENGTH;
	} else {
		retval = -ENOMEM;
		TS_LOG_ERR("%s(%d), g_td43xx_rt95_part_two is NULL\n", __func__, __LINE__);
		goto exit;
	}
	for (i = 0; i < tx_num; i++) {
		for (j = 0; j < rx_num; j++) {
			if (i < right_size) {
				temp = (unsigned int) image[i * rx_num + j];
				temp = temp * NORMAL_NUM_ONE_HUNDRED / right_median[j];	/*100 for avoid decimal number*/
				snprintf(buf, sizeof(buf),"%3d,", temp);
				lens += sizeof(buf);
				if (lens > tx_rx_delta_size) {
					retval = -ENOMEM;
					TS_LOG_ERR("%s(%d), lens is %d larger than buffer\n", __func__, __LINE__,lens);
					goto exit;
				}
				strncat(g_td43xx_rt95_part_two, buf ,sizeof(buf));
				if (!((j+EESHORT_PRINT_SHIFT_LENGTH)%rx_num)) {
					lens += EESHORT_PRINT_SHIFT_LENGTH;
					if (lens > tx_rx_delta_size) {
						retval = -ENOMEM;
						TS_LOG_ERR("%s(%d), lens is %d larger than buffer\n", __func__, __LINE__,lens);
						goto exit;
					}
					strncat(g_td43xx_rt95_part_two, "\n", EESHORT_PRINT_SHIFT_LENGTH);
				}
			}
			else {
				temp = (unsigned int) image[i * rx_num + j];
				temp = temp * NORMAL_NUM_ONE_HUNDRED / left_median[j]; /*100 for avoid decimal number*/
				snprintf(buf, sizeof(buf),"%3d,", temp);
				lens += sizeof(buf);
				if (lens > tx_rx_delta_size) {
					retval = -ENOMEM;
					TS_LOG_ERR("%s(%d), lens is %d larger than buffer\n", __func__, __LINE__,lens);
					goto exit;
				}
				strncat(g_td43xx_rt95_part_two, buf ,sizeof(buf));
				if (!((j+EESHORT_PRINT_SHIFT_LENGTH)%rx_num)) {
					lens += EESHORT_PRINT_SHIFT_LENGTH;
					if (lens > tx_rx_delta_size) {
						retval = -ENOMEM;
						TS_LOG_ERR("%s(%d), lens is %d larger than buffer\n", __func__, __LINE__,lens);
						goto exit;
					}
					strncat(g_td43xx_rt95_part_two, "\n", EESHORT_PRINT_SHIFT_LENGTH);
				}
			}
			if (temp < part_two_limit) {
				image[i * rx_num + j] = 1;
			}
			else {
				image[i * rx_num + j] = 0;
			}
		}
	}

exit:
	if (right_median)
		kfree(right_median);
	if (left_median)
		kfree(left_median);
	if (right_column_buf)
		kfree(right_column_buf);
	if (left_column_buf)
		kfree(left_column_buf);
	return retval;
}

static int td43xx_ee_short_report(void)
{
	int i = 0, j = 0, k = 0;
	int tx_num = f54->rmi4_data->num_of_tx;
	int rx_num = f54->rmi4_data->num_of_rx;
	signed short *td43xx_rt95_part_one = NULL;
	signed short *td43xx_rt95_part_two = NULL;
	char *td43xx_ee_short_data = NULL;
	unsigned int td43xx_rt95_report_size = tx_num * rx_num * 2;
	int TestResult = TEST_PASS;
	int retval = 0;
	int part_one_limit = f54->rmi4_data->synaptics_chip_data->tddi_ee_short_test_partone_limit;
	int part_two_limit = f54->rmi4_data->synaptics_chip_data->tddi_ee_short_test_parttwo_limit;
	unsigned char *buffer = f54->report_data;
	struct ts_rawdata_limit_tab limit_tab = {0};

	char buf[MAX_CAP_DATA_SIZE_FOR_EESHORT] = {0};
	int tx_rx_delta_size = 0;
	int lens = 0;
	if (!buffer) {
		TS_LOG_ERR("mmi_buf data is NULL\n");
		return TEST_FAIL;
	}

	if (f54->rmi4_data->synaptics_chip_data->test_capacitance_via_csvfile) {
		limit_tab.unique_test = kzalloc(sizeof(struct ts_unique_capacitance_test), GFP_KERNEL);
		if (!limit_tab.unique_test) {
			TS_LOG_ERR("ts_unique_capacitance_test kzalloc error\n");
			TestResult = TEST_FAIL;
			goto exit;
		}
		limit_tab.unique_test->ee_short_data_limit =
				(int32_t*)kzalloc(EE_SHORT_DATA_LIMIT_SIZE * sizeof(int32_t), GFP_KERNEL);
		if (!limit_tab.unique_test->ee_short_data_limit) {
			TS_LOG_ERR("ee_short_data_limit buffer kzalloc error\n");
			TestResult = TEST_FAIL;
			goto exit;
		}
		if (TEST_FAIL == synaptics_get_threshold_from_csvfile(CSVFILE_EE_SHORT_COLUMN,
			CSVFILE_EE_SHORT_ROW, CSV_TP_EE_SHORT_DATA_LIMIT, limit_tab.unique_test->ee_short_data_limit)) {
			TS_LOG_ERR("csvfile get ee_short_data_limit err, use dts config.\n");
		} else {
			part_one_limit = limit_tab.unique_test->ee_short_data_limit[LIMIT_ONE];
			part_two_limit = limit_tab.unique_test->ee_short_data_limit[LIMIT_TWO];
		}
	}

	TS_LOG_INFO("%s: report_type=%d,tx=%d,rx=%d,limit(%d, %d) #%d\n",
		__func__, f54->report_type, tx_num, rx_num, part_one_limit, part_two_limit,__LINE__);
	if (!part_one_limit || !part_two_limit) {
		TS_LOG_ERR("td43xx ee_short test limit is NULL\n");
		TestResult = TEST_FAIL;
		goto exit;
	}

	td43xx_ee_short_data = kzalloc(tx_num * rx_num, GFP_KERNEL);
	if (!td43xx_ee_short_data) {
		TS_LOG_ERR("%s(%d), failed to malloc td43xx_ee_short_data\n", __func__, __LINE__);
		TestResult = TEST_FAIL;
		goto exit;
	}

	td43xx_rt95_part_one = kzalloc(td43xx_rt95_report_size, GFP_KERNEL);
	if (!td43xx_rt95_part_one) {
		TS_LOG_ERR("%s(%d), failed to malloc td43xx_rt95_part_one\n", __func__, __LINE__);
		TestResult = TEST_FAIL;
		goto exit;
	}

	td43xx_rt95_part_two = kzalloc(td43xx_rt95_report_size, GFP_KERNEL);
	if (!td43xx_rt95_part_two) {
		TS_LOG_ERR("%s(%d), failed to malloc td43xx_rt95_part_two\n", __func__, __LINE__);
		TestResult = TEST_FAIL;
		goto exit;
	}
	if (NULL != g_td43xx_rt95_part_one) {
		if (!tx_num || !rx_num) {
			TS_LOG_ERR("%s(%d),tx_num=%d rx_num=%d\n", __func__, __LINE__,tx_num,rx_num);
			TestResult = TEST_FAIL;
			goto exit;
		} else {
			tx_rx_delta_size = (TX_RX_BUF_MAX<=(tx_num*rx_num*MAX_CAP_DATA_SIZE))?TX_RX_BUF_MAX:(tx_num*rx_num*MAX_CAP_DATA_SIZE);
		}
		snprintf(g_td43xx_rt95_part_one, PART_ONE_LIMIT_LENGTH, "%s\n", "part_one:");
		lens += PART_ONE_LIMIT_LENGTH;
	} else {
		TestResult = TEST_FAIL;
		TS_LOG_ERR("%s(%d), g_td43xx_rt95_part_one is NULL\n", __func__, __LINE__);
		goto exit;
	}
	for (i = 0, k = 0; i < tx_num * rx_num; i++) {
		td43xx_rt95_part_one[i] = buffer[k] |
								(buffer[k + 1]) << SHIFT_ONE_BYTE;
		snprintf(buf, sizeof(buf),"%3d,", td43xx_rt95_part_one[i]);
		lens += sizeof(buf);
		if (lens > tx_rx_delta_size) {
			TestResult = TEST_FAIL;
			TS_LOG_ERR("%s(%d), lens is %d larger than buffer\n", __func__, __LINE__,lens);
			goto exit;
		}
		strncat(g_td43xx_rt95_part_one, buf ,sizeof(buf));
		if (!((i+EESHORT_PRINT_SHIFT_LENGTH)%rx_num)) {
			lens += EESHORT_PRINT_SHIFT_LENGTH;
			if (lens > tx_rx_delta_size) {
				TestResult = TEST_FAIL;
				TS_LOG_ERR("%s(%d), lens is %d larger than buffer\n", __func__, __LINE__,lens);
				goto exit;
			}
			strncat(g_td43xx_rt95_part_one, "\n", EESHORT_PRINT_SHIFT_LENGTH);
		}
		td43xx_rt95_part_one[i] = (td43xx_rt95_part_one[i] > part_one_limit) ? EE_SHORT_TEST_FAIL : EE_SHORT_TEST_PASS;
		k += NORMAL_NUM_TWO;
	}

	for (i = 0, k = td43xx_rt95_report_size; i < tx_num * rx_num; i++) {
		td43xx_rt95_part_two[i] = buffer[k] |
								(buffer[k + 1]) << SHIFT_ONE_BYTE;
		k += NORMAL_NUM_TWO;
	}

	retval = td43xx_ee_short_normalize_data(td43xx_rt95_part_two);
	if (retval < 0) {
		TS_LOG_ERR("%s(%d), td43xx_ee_short_normalize_data failed\n", __func__, __LINE__);
		TestResult = TEST_FAIL;
		goto exit;
	}

	for (i = 0; i < tx_num; i++) {
		for (j = 0; j < rx_num; j++) {
			td43xx_ee_short_data[i*rx_num +j] =
			(unsigned char)(td43xx_rt95_part_one[i * rx_num + j]) ||
				td43xx_rt95_part_two[i * rx_num + j];
			if (0 != td43xx_ee_short_data[i * rx_num + j]) {
				TestResult = TEST_FAIL;
				test_failed_reason[RAW_DATA_TYPE_OpenShort] = TEST_THRESHOLD_FAIL;
				TS_LOG_ERR("td43xx_ee_short_data:[%d, %d]%d\n",
					i, j, td43xx_ee_short_data[i * tx_num + j]);
			}
		}
	}

exit:
	if (limit_tab.unique_test && limit_tab.unique_test->ee_short_data_limit){
		kfree(limit_tab.unique_test->ee_short_data_limit);
		limit_tab.unique_test->ee_short_data_limit = NULL;
	}
	if (limit_tab.unique_test) {
		kfree(limit_tab.unique_test);
		limit_tab.unique_test = NULL;
	}
	if (td43xx_rt95_part_one)
		kfree(td43xx_rt95_part_one);
	if (td43xx_rt95_part_two)
		kfree(td43xx_rt95_part_two);
	if (td43xx_ee_short_data)
		kfree(td43xx_ee_short_data);

	return TestResult;
}

static void mmi_trex_shorts_test(void)
{
	unsigned char command = 0;
	unsigned char i = 0;
	int result = 0;
	char *buf_backup = NULL;

	if(NULL == f54) {
		TS_LOG_ERR("%s: f54 is NULL\n", __func__);
		goto param_nul;
	}
	if(NULL == f54->rmi4_data) {
		TS_LOG_ERR("%s: f54->rmi4_data is NULL\n", __func__);
		goto param_nul;
	}
	if(NULL == f54->rmi4_data->synaptics_chip_data) {
		TS_LOG_ERR("%s: f54->rmi4_data->synaptics_chip_data is NULL\n", __func__);
		goto param_nul;
	}
	command = (unsigned char)F54_TREX_SHORTS;
	TS_LOG_INFO("mmi_trex_shorts_test called\n");
	write_to_f54_register(command);
	f54->report_type = command;
	result = synaptics_rmi4_f54_attention();
	if (result < 0) {
		TS_LOG_ERR("%s: Failed to get data\n", __func__);
		goto err_get_result;
	}
	TS_LOG_INFO("%s-%d: trex_shorts_report_size = %d.\n", __func__, __LINE__, f54->report_size);

	if(NULL == f54->mmi_buf) {
		TS_LOG_ERR("%s: f54->mmi_buf is NULL\n", __func__);
		goto err_mmi_buf_null;
	}
	buf_backup = kzalloc(f54->report_size, GFP_KERNEL);
	if(!buf_backup) {
		TS_LOG_ERR("%s: buf_backup kzalloc failed\n", __func__);
		goto err_kzalloc_buf;
	}
	memcpy(buf_backup, f54->mmi_buf, f54->report_size);
	if(f54->report_size < TP_3320_SHORT_ARRAY_NUM) {
		TS_LOG_ERR("%s: report_size < trx_short_array_num, err\n", __func__);
		goto err_report_size;
	}
	for(i = 0; i < TP_3320_SHORT_ARRAY_NUM; i++) {
		TS_LOG_INFO("%s: buf_backup[%d] is %d\n", __func__, i, buf_backup[i]);
		if(buf_backup[i] != f54->rmi4_data->synaptics_chip_data->trx_short_circuit_array[i]) {
			TS_LOG_ERR("%s: test result is failed\n", __func__);
			goto err_match_defult;
		}
	}
	TS_LOG_INFO("%s: test result is succ\n", __func__);
	strncat(buf_f54test_result, SHORT_TEST_PASS, MAX_STR_LEN);
	kfree(buf_backup);
	return;

err_match_defult:
	test_failed_reason[RAW_DATA_TYPE_OpenShort] = TEST_THRESHOLD_FAIL;
err_report_size:
	kfree(buf_backup);
err_kzalloc_buf:
err_mmi_buf_null:
err_get_result:
param_nul:
	strncat(buf_f54test_result, SHORT_TEST_FAIL, MAX_STR_LEN);
	return;
}

static void mmi_td43xx_ee_short_report(void)
{
	unsigned char command;
	int result = 0;
	command = (unsigned char)F54_TD43XX_EE_SHORT;

	TS_LOG_INFO("mmi_deltacapacitance_test called\n");
	write_to_f54_register(command);
	f54->report_type = command;
	result = synaptics_rmi4_f54_attention();
	if (result < 0) {
		TS_LOG_ERR("Failed to get data\n");
		strncat(buf_f54test_result, TD43XX_EE_SHORT_TEST_FAIL, MAX_STR_LEN);
		return;
	}
	result = td43xx_ee_short_report();
	if (TEST_PASS == result) {
		TS_LOG_INFO("tdxx_ee_short test is successed, result: 5P\n");
		strncat(buf_f54test_result, TD43XX_EE_SHORT_TEST_PASS, MAX_STR_LEN);
	} else {
		strncat(buf_f54test_result, TD43XX_EE_SHORT_TEST_FAIL, MAX_STR_LEN);
		strncpy(tp_test_failed_reason, "-panel_reason",
			TP_TEST_FAILED_REASON_LEN);
		TS_LOG_INFO("tdxx_ee_short test is failed, result: 5F\n");
	}

	return;
}
static int f54_delta_rx_report(void)
{
	short Rawimage_rx;
	short Rawimage_rx1;
	short Rawimage_rx2;
	short Result = 0;
	int i = 0;
	int j = 0;
	int step = 0;
	int delt_cap_uplimit =
	    f54->rmi4_data->synaptics_chip_data->raw_limit_buf[RX_TO_RX_UP];
	int delt_cap_lowlimit =
	    f54->rmi4_data->synaptics_chip_data->raw_limit_buf[RX_TO_RX_LOW];

	TS_LOG_INFO
	    ("rx = %d, tx = %d, delt_cap_uplimit = %d, delt_cap_lowlimit = %d\n",
	     f54->rmi4_data->num_of_rx, f54->rmi4_data->num_of_tx,
	     delt_cap_uplimit, delt_cap_lowlimit);

	for (i = 0; i < mmi_buf_size; i += 2) {
		Rawimage_rx1 = (f54->mmi_buf[i]) | (f54->mmi_buf[i + 1] << 8);
		Rawimage_rx2 =
		    (f54->mmi_buf[i + 2]) | (f54->mmi_buf[i + 3] << 8);
		Rawimage_rx = Rawimage_rx2 - Rawimage_rx1;
		if (synaptics_raw_data_limit_flag) {
			delt_cap_uplimit = RxDeltaCapUpperLimit[i / 2 - step];
			delt_cap_lowlimit = RxDeltaCapLowerLimit[i / 2 - step];
		}
		if ((Rawimage_rx <= delt_cap_uplimit)
		    && (Rawimage_rx >= delt_cap_lowlimit)) {
			Result++;
		} else {
			TS_LOG_INFO("[%d,%d]\n", i / 2 - step, Rawimage_rx);
		}
		j++;
		if (j == f54->rmi4_data->num_of_rx - 1) {
			i += 2;
			j = 0;
			step += 1;
		}
	}
	if (Result == (mmi_buf_size / 2 - f54->rmi4_data->num_of_tx)) {
		TS_LOG_DEBUG("rawdata rx diff is all right, Result = %d\n",
			     Result);
		return 1;
	} else {
		TS_LOG_ERR("rawdata rx diff is out of range, Result = %d\n",
			   Result);
		return 0;
	}

}

static int f54_delta_tx_report(void)
{
	short *Rawimage_tx = NULL;
	short Rawimage_delta_tx;
	int i, j, tx_n, rx_n;
	int Result = 0;
	int tx_to_tx_cap_uplimit =
	    f54->rmi4_data->synaptics_chip_data->raw_limit_buf[TX_TO_TX_UP];
	int tx_to_tx_cap_lowlimit =
	    f54->rmi4_data->synaptics_chip_data->raw_limit_buf[TX_TO_TX_LOW];

	TS_LOG_INFO
	    ("rx = %d, tx = %d, tx_to_tx_cap_uplimit = %d, tx_to_tx_cap_lowlimit = %d\n",
	     f54->rmi4_data->num_of_rx, f54->rmi4_data->num_of_tx,
	     tx_to_tx_cap_uplimit, tx_to_tx_cap_lowlimit);

	Rawimage_tx = (short *)kzalloc(mmi_buf_size, GFP_KERNEL);
	if (!Rawimage_tx) {
		TS_LOG_ERR("Rawimage_tx kzalloc error\n");
		return 0;
	}

	for (i = 0, j = 0; i < mmi_buf_size; i += 2, j++)
		Rawimage_tx[j] = (f54->mmi_buf[i]) | (f54->mmi_buf[i + 1] << 8);

	for (tx_n = 0; tx_n < f54->rmi4_data->num_of_tx - 1; tx_n++) {
		for (rx_n = 0; rx_n < f54->rmi4_data->num_of_rx; rx_n++) {
			if (synaptics_raw_data_limit_flag) {
				tx_to_tx_cap_uplimit =
				    TxDeltaCapUpperLimit[tx_n *
							 f54->rmi4_data->
							 num_of_rx + rx_n];
				tx_to_tx_cap_lowlimit =
				    TxDeltaCapLowerLimit[tx_n *
							 f54->rmi4_data->
							 num_of_rx + rx_n];
			}
			Rawimage_delta_tx =
			    Rawimage_tx[(tx_n + 1) * f54->rmi4_data->num_of_rx +
					rx_n] -
			    Rawimage_tx[tx_n * f54->rmi4_data->num_of_rx +
					rx_n];
			if ((Rawimage_delta_tx <= tx_to_tx_cap_uplimit)
			    && (Rawimage_delta_tx >= tx_to_tx_cap_lowlimit)) {
				Result++;
			} else {
				TS_LOG_INFO("[%d,%d]\n",
					    tx_n * f54->rmi4_data->num_of_rx +
					    rx_n, Rawimage_delta_tx);
			}
		}
	}
	kfree(Rawimage_tx);

	if (Result == (mmi_buf_size / 2 - f54->rmi4_data->num_of_rx)) {
		TS_LOG_DEBUG("rawdata tx diff is all right, Result = %d\n",
			     Result);
		return 1;
	} else {
		TS_LOG_ERR("rawdata tx diff is out of range, Result = %d\n",
			   Result);
		return 0;
	}
}
static int synaptics_get_threshold_from_csvfile(int columns, int rows, char* target_name, int32_t *data)
{
	char file_path[100] = {0};
	char file_name[64] = {0};
	int ret = 0;
	int result = 0;
	TS_LOG_INFO("%s called\n", __func__);

	if (!data || !target_name) {
		TS_LOG_ERR("parse csvfile failed, data or target_name is NULL\n");
		return TEST_FAIL;
	}

	if (!strnlen(f54->rmi4_data->synaptics_chip_data->ts_platform_data->product_name, MAX_STR_LEN-1)
		|| !strnlen(f54->rmi4_data->synaptics_chip_data->chip_name, MAX_STR_LEN-1)
		|| !strnlen(f54->rmi4_data->rmi4_mod_info.project_id_string, SYNAPTICS_RMI4_PROJECT_ID_SIZE)
		|| f54->rmi4_data->module_name == NULL) {
		TS_LOG_INFO("Threshold file name is not detected\n");
		return TEST_FAIL;
	}

	snprintf(file_name, sizeof(file_name), "%s_%s_%s_%s_raw.csv",
			f54->rmi4_data->synaptics_chip_data->ts_platform_data->product_name,
			f54->rmi4_data->synaptics_chip_data->chip_name,
			f54->rmi4_data->rmi4_mod_info.project_id_string,
			f54->rmi4_data->module_name);
	if (CSV_PRODUCT_SYSTEM == f54->rmi4_data->synaptics_chip_data->csvfile_use_product_system) {
		snprintf(file_path, sizeof(file_path), "/product/etc/firmware/ts/%s", file_name);
	}
	else if (CSV_ODM_SYSTEM == f54->rmi4_data->synaptics_chip_data->csvfile_use_product_system) {
		snprintf(file_path, sizeof(file_path), "/odm/etc/firmware/ts/%s", file_name);
	}
	else {
		TS_LOG_ERR("csvfile path is not supported, csvfile_use_product_system = %d\n",
				f54->rmi4_data->synaptics_chip_data->csvfile_use_product_system);
		return TEST_FAIL;
	}
	TS_LOG_INFO("threshold file name:%s, rows_size=%d, columns_size=%d\n", file_path, rows, columns);

	result =  ts_kit_parse_csvfile(file_path, target_name, data, rows, columns);
	if (0 == result){
		ret = TEST_PASS;
		TS_LOG_INFO(" Get threshold successed form csvfile\n");
	} else {
		TS_LOG_ERR("csv file parse fail:%s |ret = %d\n", file_path,result);
		ret = TEST_FAIL;
	}
	return ret;
}
static int f54_delta_rx2_report(void)
{
	short Rawimage_rx = 0;
	short Rawimage_rx1 = 0;
	short Rawimage_rx2 = 0;
	short Result = 0;
	int i = 0;
	int j = 0;
	int delta_buf_count = 0;
	int step = 0;
	int ret = 0;
	short rxdelt_cap_abslimit = 0;
	char buf[MAX_CAP_DATA_SIZE] = {0};
	struct ts_rawdata_limit_tab limit_tab = {0};
	int rows_size = f54->rmi4_data->num_of_tx;
	int columns_size = f54->rmi4_data->num_of_rx;
	int *abs_rxdelt_cap_limit = NULL;

	TS_LOG_INFO("%s called\n", __func__);

	limit_tab.unique_test = kzalloc(sizeof(struct ts_unique_capacitance_test), GFP_KERNEL);
	if (!limit_tab.unique_test) {
		TS_LOG_ERR("ts_unique_capacitance_test kzalloc error\n");
		ret = TEST_FAIL;
		goto error_release_mem;
	}
	limit_tab.unique_test->Read_only_support_unique = f54->rmi4_data->synaptics_chip_data->trx_delta_test_support;
	limit_tab.unique_test->Rx_delta_abslimit =
			(int32_t*)kzalloc(rows_size*columns_size*sizeof(int32_t), GFP_KERNEL);
	if (!limit_tab.unique_test->Rx_delta_abslimit) {
		TS_LOG_ERR("rx_delta_abslimit buffer kzalloc error\n");
		ret = TEST_FAIL;
		goto error_release_mem;
	}
	if (TEST_FAIL == synaptics_get_threshold_from_csvfile(columns_size-1, rows_size, CSV_TP_DELTA_ABS_RX_LIMIT, limit_tab.unique_test->Rx_delta_abslimit)) {
		TS_LOG_ERR("get abs_rxdelt_cap_limit err\n");
		ret = TEST_FAIL;
		memset(limit_tab.unique_test->Rx_delta_abslimit, 0, rows_size*columns_size*sizeof(int32_t));
	}
	abs_rxdelt_cap_limit = limit_tab.unique_test->Rx_delta_abslimit;

	if(f54->rmi4_data->synaptics_chip_data->rawdata_newformatflag == TS_RAWDATA_NEWFORMAT) {
		for ( i = 0; i < mmi_buf_size; i+=2) {
					Rawimage_rx1 = (f54->mmi_buf[i]) | (f54->mmi_buf[i+1] << SHIFT_ONE_BYTE);
					Rawimage_rx2 = (f54->mmi_buf[i+2]) | (f54->mmi_buf[i+3] << SHIFT_ONE_BYTE);
					Rawimage_rx = abs(Rawimage_rx2 - Rawimage_rx1);
			rx_delta_buf[delta_buf_count] = Rawimage_rx;
			delta_buf_count++;
			rxdelt_cap_abslimit = abs_rxdelt_cap_limit[i/2 - step];
			if (Rawimage_rx <= rxdelt_cap_abslimit){
				Result++;
			} else {
				TS_LOG_ERR("[%d,%d] %d\n",i/2 - step,Rawimage_rx, rxdelt_cap_abslimit);
			}
			j++;
			if (j == columns_size - 1) {
				i += 2;
				j = 0;
				step += 1;
			}
		}
	} else {
		snprintf(rx_delta_buf, MAX_CAP_DATA_SIZE, "%s\n", "RX:");
		for ( i = 0; i < mmi_buf_size; i+=2)
		{
			Rawimage_rx1 = (f54->mmi_buf[i]) | (f54->mmi_buf[i+1] << SHIFT_ONE_BYTE);
			Rawimage_rx2 = (f54->mmi_buf[i+2]) | (f54->mmi_buf[i+3] << SHIFT_ONE_BYTE);
			Rawimage_rx = abs(Rawimage_rx2 - Rawimage_rx1);
			snprintf(buf, sizeof(buf),"%3d,", Rawimage_rx);
			strncat(rx_delta_buf, buf ,sizeof(buf));
			rxdelt_cap_abslimit = abs_rxdelt_cap_limit[i/2 - step];
			if (Rawimage_rx <= rxdelt_cap_abslimit){
				Result++;
			}
			else{
				TS_LOG_ERR("[%d,%d] %d\n",i/2 - step,Rawimage_rx, rxdelt_cap_abslimit);
			}
			j++;
			if (j == columns_size - 1) {
				i += 2;
				j = 0;
				step += 1;
				strncat(rx_delta_buf, "\n", 1);
			}
		}
	}
	if (Result == (mmi_buf_size/2 - rows_size)) {
		TS_LOG_INFO("rawdata rx diff is all right, Result = %d\n", Result);
		ret = TEST_PASS;
	}
	else {
		TS_LOG_ERR("rawdata rx diff is out of range, Result = %d\n", Result);
		ret = TEST_FAIL;
		test_failed_reason[RAW_DATA_TYPE_TrxDelta] = TEST_THRESHOLD_FAIL;
	}
error_release_mem:
	if (limit_tab.unique_test && limit_tab.unique_test->Rx_delta_abslimit){
		kfree(limit_tab.unique_test->Rx_delta_abslimit);
		limit_tab.unique_test->Rx_delta_abslimit = NULL;
	}
	if (limit_tab.unique_test) {
		kfree(limit_tab.unique_test);
		limit_tab.unique_test = NULL;
	}

	return ret;
}

static int f54_delta_tx2_report(void)
{
	int i = 0, j = 0;
	int tx_n = 0, rx_n = 0;
	int Result=0;
	int ret = 0;
	int delta_buf_count = 0;
	short txdelt_cap_abslimit = 0;
	short Rawimage_delta_tx = 0;
	char buf[MAX_CAP_DATA_SIZE] = {0};
	struct ts_rawdata_limit_tab limit_tab = {0};
	int rows_size = f54->rmi4_data->num_of_tx;
	int columns_size = f54->rmi4_data->num_of_rx;
	int *abs_txdelt_cap_limit =NULL;
	short *Rawimage_tx = NULL;

	TS_LOG_INFO("%s called\n", __func__);

	limit_tab.unique_test = kzalloc(sizeof(struct ts_unique_capacitance_test), GFP_KERNEL);
	if (!limit_tab.unique_test) {
		TS_LOG_ERR("ts_unique_capacitance_test kzalloc error\n");
		ret = TEST_FAIL;
		goto error_release_mem;
	}
	limit_tab.unique_test->Read_only_support_unique = f54->rmi4_data->synaptics_chip_data->trx_delta_test_support;
	limit_tab.unique_test->Tx_delta_abslimit =
			(int32_t*)kzalloc(rows_size*columns_size*sizeof(int32_t), GFP_KERNEL);
	if (!limit_tab.unique_test->Tx_delta_abslimit) {
		TS_LOG_ERR("Tx_delta_abslimit buffer kzalloc error\n");
		ret = TEST_FAIL;
		goto error_release_mem;
	}
	if (TEST_FAIL == synaptics_get_threshold_from_csvfile(columns_size, rows_size-1, CSV_TP_DELTA_ABS_TX_LIMIT, limit_tab.unique_test->Tx_delta_abslimit)) {
		TS_LOG_ERR("get abs_txdelt_cap_limit err\n");
		ret = TEST_FAIL;
		memset(limit_tab.unique_test->Tx_delta_abslimit, 0, rows_size*columns_size*sizeof(int32_t));
	}
	abs_txdelt_cap_limit = limit_tab.unique_test->Tx_delta_abslimit;

	Rawimage_tx = (short *)kzalloc(mmi_buf_size, GFP_KERNEL);
	if (!Rawimage_tx) {
		TS_LOG_ERR("Rawimage_tx kzalloc error\n");
		ret = TEST_FAIL;
		goto error_release_mem;
	}
	if(f54->rmi4_data->synaptics_chip_data->rawdata_newformatflag == TS_RAWDATA_NEWFORMAT) {
		for ( i = 0, j = 0; i < mmi_buf_size; i+=2, j++)
					Rawimage_tx[j] = (f54->mmi_buf[i]) | (f54->mmi_buf[i+1] << SHIFT_ONE_BYTE);
		for( tx_n = 0; tx_n < rows_size - 1; tx_n++) {
			for(rx_n = 0; rx_n < columns_size; rx_n++) {
				txdelt_cap_abslimit = abs_txdelt_cap_limit[tx_n*columns_size+rx_n];
				Rawimage_delta_tx = abs(Rawimage_tx[(tx_n+1)*columns_size+rx_n]
						- Rawimage_tx[tx_n*columns_size+rx_n]);
				tx_delta_buf[delta_buf_count] = Rawimage_delta_tx;
				delta_buf_count++;

				if(Rawimage_delta_tx <= txdelt_cap_abslimit){
					Result++;
				}
				else{
					TS_LOG_ERR("[%d,%d]\n",tx_n*columns_size+rx_n,Rawimage_delta_tx);
				}
			}
		}

	}else{

		snprintf(tx_delta_buf, MAX_CAP_DATA_SIZE, "\n%s\n", "TX:");
		for ( i = 0, j = 0; i < mmi_buf_size; i+=2, j++)
			Rawimage_tx[j] = (f54->mmi_buf[i]) | (f54->mmi_buf[i+1] << SHIFT_ONE_BYTE);

		for( tx_n = 0; tx_n < rows_size - 1; tx_n++)
		{
			for(rx_n = 0; rx_n < columns_size; rx_n++)
			{
				txdelt_cap_abslimit = abs_txdelt_cap_limit[tx_n*columns_size+rx_n];
				Rawimage_delta_tx = abs(Rawimage_tx[(tx_n+1)*columns_size+rx_n]
					- Rawimage_tx[tx_n*columns_size+rx_n]);
				snprintf(buf, sizeof(buf),"%3d,", Rawimage_delta_tx);
				strncat(tx_delta_buf, buf ,sizeof(buf));
				if(Rawimage_delta_tx <= txdelt_cap_abslimit){
					Result++;
				}
				else{
					TS_LOG_ERR("[%d,%d]\n",tx_n*columns_size+rx_n,Rawimage_delta_tx);
				}
			}
			strncat(tx_delta_buf, "\n", 1);
		}
	}
	if (Result == (mmi_buf_size/2 - columns_size)) {
		TS_LOG_INFO("rawdata tx diff is all right, Result = %d\n", Result);
		ret = TEST_PASS;
	}
	else {
		TS_LOG_ERR("rawdata tx diff is out of range, Result = %d\n", Result);
		ret = TEST_FAIL;
		test_failed_reason[RAW_DATA_TYPE_TrxDelta] = TEST_THRESHOLD_FAIL;
	}
error_release_mem:
	if (Rawimage_tx) {
		kfree(Rawimage_tx);
		Rawimage_tx = NULL;
	}
	if (limit_tab.unique_test && limit_tab.unique_test->Tx_delta_abslimit) {
		kfree(limit_tab.unique_test->Tx_delta_abslimit);
		limit_tab.unique_test->Tx_delta_abslimit = NULL;
	}
	if (limit_tab.unique_test) {
		kfree(limit_tab.unique_test);
		limit_tab.unique_test = NULL;
	}
	return ret;
}

static void mmi_rawcapacitance_test(void)
{
	unsigned char command;
	int result = 0;

	if (20 == f54->rmi4_data->synaptics_chip_data->rawdata_report_type)
		command = (unsigned char)F54_FULL_RAW_CAP_RX_COUPLING_COMP;
	else
		command = (unsigned char)F54_RAW_16BIT_IMAGE;

	TS_LOG_INFO("mmi_rawcapacitance_test called, command is %d\n", command);

	write_to_f54_register(command);
	f54->report_type = command;
	result = synaptics_rmi4_f54_attention();
	if (result < 0) {
		TS_LOG_ERR("Failed to get data\n");
		strncat(buf_f54test_result, "1F", MAX_STR_LEN);
		return;
	}
	result = f54_rawimage_report();
	if (1 == result) {
		strncat(buf_f54test_result, "1P", MAX_STR_LEN);
	} else {
		TS_LOG_ERR("raw data is out of range, , test result is 1F\n");
		strncat(buf_f54test_result, "1F", MAX_STR_LEN);
		strncpy(tp_test_failed_reason, "-panel_reason",
			TP_TEST_FAILED_REASON_LEN);
	}
	if (1 == (f54_delta_rx_report() && f54_delta_tx_report())) {
		strncat(buf_f54test_result, "-2P", MAX_STR_LEN);
	} else {
		TS_LOG_ERR
		    ("raw data diff is out of range, test result is 2F\n");
		strncat(buf_f54test_result, "-2F", MAX_STR_LEN);
		strncpy(tp_test_failed_reason, "-panel_reason",
			TP_TEST_FAILED_REASON_LEN);
	}
	return;
}

static void save_capacitance_data_to_rawdatabuf(void)
{
	int i , j;
	short temp;
	int index = 0;

	f54->rawdatabuf[0] = f54->rmi4_data->num_of_rx;
	f54->rawdatabuf[1] = f54->rmi4_data->num_of_tx;
	for(i = 0, j = index + 2; i < mmi_buf_size; i+=2, j++) {
		temp = (f54->mmi_buf[i]) | (f54->mmi_buf[i+1] << 8);
		f54->rawdatabuf[j] = temp;
	}
}

static int get_int_average(int *p, size_t len)
{
	int sum = 0;
	size_t i;

	for (i = 0; i < len; i++) {
		sum += p[i];
	}
	if (len != 0) {
		return (sum / len);
	} else {
		return 0;
	}
}

static int get_int_min(int *p, size_t len)
{
	int s_min = SHRT_MAX;
	size_t i;

	for (i = 0; i < len; i++) {
		s_min = s_min > p[i] ? p[i] : s_min;
	}

	return s_min;
}

static int get_int_max(int *p, size_t len)
{
	int s_max = SHRT_MIN;
	size_t i;

	for (i = 0; i < len; i++) {
		s_max = s_max < p[i] ? p[i] : s_max;
	}

	return s_max;
}

static void get_capacitance_stats(void)
{
	size_t num_elements = f54->rawdatabuf[0] * f54->rawdatabuf[1];

	f54->raw_statics_data.RawimageAverage =
		get_int_average(&f54->rawdatabuf[2], num_elements);
	f54->raw_statics_data.RawimageMaxNum =
		get_int_max(&f54->rawdatabuf[2], num_elements);
	f54->raw_statics_data.RawimageMinNum =
		get_int_min(&f54->rawdatabuf[2], num_elements);
}

static int check_enhance_raw_capacitance(void)
{
	int i;
	int count = f54->rmi4_data->num_of_rx * f54->rmi4_data->num_of_tx;

	save_capacitance_data_to_rawdatabuf();
	get_capacitance_stats();

	for (i = 0; i < count; i++) {
		TS_LOG_INFO("rawdata is upper: %d, lower: %d\n", f54->rmi4_data->synaptics_chip_data->upper[i], f54->rmi4_data->synaptics_chip_data->lower[i]);
		/* rawdatabuf[0] rawdatabuf[1] are num_of_rx and num_of_tx */
		if ((f54->rawdatabuf[i+2] > f54->rmi4_data->synaptics_chip_data->upper[i]) || (f54->rawdatabuf[i+2] < f54->rmi4_data->synaptics_chip_data->lower[i])) {
			TS_LOG_ERR("rawdata is out of range, failed at %d: upper: %d, lower: %d, raw: %d\n", i, f54->rmi4_data->synaptics_chip_data->upper[i], f54->rmi4_data->synaptics_chip_data->lower[i], f54->rawdatabuf[i+2]);
			return 0;
		}
	}

	return 1;
}

static int check_csvfile_raw_capacitance(void)
{
	int i = 0;
	int ret = TEST_PASS;
	int count = f54->rmi4_data->num_of_rx * f54->rmi4_data->num_of_tx;
	struct ts_rawdata_limit_tab limit_tab = {0};
	int rows_size = f54->rmi4_data->num_of_tx;
	int columns_size = f54->rmi4_data->num_of_rx;

	TS_LOG_INFO("%s called, rows:%d, columns:%d\n", __func__, rows_size, columns_size);
	save_capacitance_data_to_rawdatabuf();
	get_capacitance_stats();

	limit_tab.MutualRawMax =
		(int32_t*)kzalloc((rows_size)*(columns_size)*sizeof(int32_t), GFP_KERNEL);
	limit_tab.MutualRawMin =
		(int32_t*)kzalloc((rows_size)*(columns_size)*sizeof(int32_t), GFP_KERNEL);
	if (!limit_tab.MutualRawMax || !limit_tab.MutualRawMin ){
		TS_LOG_ERR("kzalloc rawdata buffer is NULL\n");
		ret = TEST_FAIL;
		goto error_release_mem;
	}
	if (TEST_FAIL == synaptics_get_threshold_from_csvfile(columns_size, rows_size, CSV_TP_CAP_RAW_MAX, limit_tab.MutualRawMax)) {
		TS_LOG_ERR("get rawdata_cap_max_limit err\n");
		ret = TEST_FAIL;
		goto error_release_mem;
	}
	if (TEST_FAIL == synaptics_get_threshold_from_csvfile(columns_size, rows_size, CSV_TP_CAP_RAW_MIN, limit_tab.MutualRawMin)) {
		TS_LOG_ERR("get rawdata_cap_min_limit err\n");
		ret = TEST_FAIL;
		goto error_release_mem;
	}

	for (i = 0; i < count; i++) {
		/* rawdatabuf[0] rawdatabuf[1] are num_of_rx and num_of_tx */
		if ((f54->rawdatabuf[i+2] > limit_tab.MutualRawMax[i]) || (f54->rawdatabuf[i+2] < limit_tab.MutualRawMin[i])) {
			TS_LOG_ERR("rawdata is out of range, failed at %d: upper: %d, lower: %d, raw: %d\n",
				i, limit_tab.MutualRawMax[i], limit_tab.MutualRawMin[i], f54->rawdatabuf[i+2]);
			ret = TEST_FAIL;
			test_failed_reason[RAW_DATA_TYPE_CAPRAWDATA] = TEST_THRESHOLD_FAIL;
		}
	}

error_release_mem:
	if (limit_tab.MutualRawMax){
		kfree(limit_tab.MutualRawMax);
		limit_tab.MutualRawMax = NULL;
	}
	if (limit_tab.MutualRawMin){
		kfree(limit_tab.MutualRawMin);
		limit_tab.MutualRawMin = NULL;
	}

	return ret;
}

static void mmi_csvfile_rawdata_test(void)
{
	unsigned char command;
	int result = 0;

	command = (unsigned char)F54_RAW_16BIT_IMAGE;

	TS_LOG_INFO("%s called, command is %d\n", __func__, command);

	write_to_f54_register(command);
	f54->report_type = command;
	result = synaptics_rmi4_f54_attention();
	if (result < 0) {
		TS_LOG_ERR("Failed to get data\n");
		strncat(buf_f54test_result, RAWDATA_CAP_TEST_FAIL, MAX_STR_LEN);
		strncpy(tp_test_failed_reason, "-panel_reason",
			TP_TEST_FAILED_REASON_LEN);
		return;
	}
	result = check_csvfile_raw_capacitance();
	if (TEST_PASS == result) {
		strncat(buf_f54test_result, RAWDATA_CAP_TEST_PASS, MAX_STR_LEN);
		TS_LOG_INFO("raw data test successed, test result is 1P\n");
	} else {
		TS_LOG_ERR("raw data is out of range, test result is 1F\n");
		strncat(buf_f54test_result, RAWDATA_CAP_TEST_FAIL, MAX_STR_LEN);
		strncpy(tp_test_failed_reason, "-panel_reason",
			TP_TEST_FAILED_REASON_LEN);
	}
	if (f54->rmi4_data->synaptics_chip_data->trx_delta_test_support) {
		if (TEST_PASS == (f54_delta_rx2_report() & f54_delta_tx2_report())) {
			strncat(buf_f54test_result, TRX_DELTA_CAP_TEST_PASS, MAX_STR_LEN);
		} else {
			TS_LOG_ERR("trx_delta test is out of range, test result is 2F\n");
			strncat(buf_f54test_result, TRX_DELTA_CAP_TEST_FAIL, MAX_STR_LEN);
			strncpy(tp_test_failed_reason,"-panel_reason",TP_TEST_FAILED_REASON_LEN);
		}
	}
	return;
}

static void mmi_enhance_raw_capacitance_test(void)
{
	unsigned char command;
	int result = 0;

	if (20 == f54->rmi4_data->synaptics_chip_data->rawdata_report_type)
		command = (unsigned char)F54_FULL_RAW_CAP_RX_COUPLING_COMP;
	else
		command = (unsigned char)F54_RAW_16BIT_IMAGE;

	TS_LOG_INFO("mmi_rawcapacitance_test called, command is %d\n", command);

	write_to_f54_register(command);
	f54->report_type = command;
	result = synaptics_rmi4_f54_attention();
	if (result < 0) {
		TS_LOG_ERR("Failed to get data\n");
		strncat(buf_f54test_result, "1F", MAX_STR_LEN);
		return;
	}
	result = check_enhance_raw_capacitance();
	if (1 == result) {
		strncat(buf_f54test_result, "1P-2P", MAX_STR_LEN);
	} else {
		TS_LOG_ERR("raw data is out of range, , test result is 1F-2P\n");
		strncat(buf_f54test_result, "1F-2P", MAX_STR_LEN);
		strncpy(tp_test_failed_reason, "-panel_reason",
			TP_TEST_FAILED_REASON_LEN);
	}
}
#define ARR_RX_SIZE 40
static unsigned char temp_data[64] = {0};
static unsigned char ExtendRT26_pin[4] = {0, 1, 32, 33};
static short maxRX[ARR_RX_SIZE] = {0};
static short minRX[ARR_RX_SIZE] = {0};
static const short limit_1 = 600;
static const short limit_2 = 550;
#define ABS(a,b) ((a - b > 0) ? a - b : b - a)

unsigned char GetLogicalPin(unsigned char p_pin)
{
	unsigned char i = 0;
	for(i = 0; i < f54->rmi4_data->num_of_rx; i++)
	{
		if (f55->rx_physical[i] == p_pin)
		  return i;
	}
	return 0xff;
}
#define EXT_TRX_SHORT_PASS 1
#define EXT_TRX_SHORT_FAIL 0
static int mmi_ext_trx_short_test(void)
{
	int retval = 0;
	int result = 0;
	int i = 0;
	int j = 0;
	int k = 0;
	int ii= 0;
	int jj = 0;
	int rxIndex = 0;
	struct synaptics_rmi4_data *rmi4_data = f54->rmi4_data;
	unsigned char logical_pin = 0xff;
	unsigned char *p_data_rt26 = NULL;
	short *p_data_baseline1 = NULL;
	short *p_data_baseline2 = NULL;
	short *p_data_delta = NULL;

	unsigned short numberOfRows = f54->rmi4_data->num_of_tx;
	unsigned short numberOfColums = f54->rmi4_data->num_of_rx;
	unsigned short temp = 0;

	unsigned char data = 0;
	unsigned char command = 0;
	TS_LOG_INFO("%s: setting enter numberofRows:%d numberofColums:%d\n", __func__, numberOfRows, numberOfColums);

	result = EXT_TRX_SHORT_PASS;

	command = (unsigned char)F54_TREX_SHORTS;
	write_to_f54_register(command);
	f54->report_type = command;
	retval = synaptics_rmi4_f54_attention();
	if (retval < NO_ERR) {
		TS_LOG_ERR("%s: do 26 type fail\n", __func__);
		goto exit_26100_type;
	}
	p_data_rt26 = kzalloc(f54->report_size,  GFP_KERNEL);
	if (!p_data_rt26) {
		TS_LOG_ERR("%s: fail to allocate p_data_rt26\n", __func__);
		goto exit_26100_type;
	}
	memcpy(p_data_rt26, f54->report_data, f54->report_size);
	for (i=0; i<f54->report_size; i++) {
		TS_LOG_INFO("RT26 data byte %d - data 0x%x\n", i, p_data_rt26[i]);
		if (p_data_rt26[i] != 0) {
			if ((i==0) && (p_data_rt26[i] & 0x01)) {
					//pin - 0
			} else if ((i==0) && (p_data_rt26[i] & 0x02)) {
					//pin - 1
			} else if ((i==4) && (p_data_rt26[i] & 0x01)) {
					//pin -32
			} else if ((i==4) && (p_data_rt26[i] & 0x02)) {
					//pin -33
			} else {
				TS_LOG_ERR("%s: RT26 test fail!!!  data byte %d - data 0x%x\n", __func__, i, p_data_rt26[i]);
				result = EXT_TRX_SHORT_FAIL;
			}
		}
	}

	//set no scan bit
	retval = f54->fn_ptr->read(rmi4_data,
			f54->control_base_addr,
			&data,
			sizeof(data));
	if (retval < NO_ERR) {
		TS_LOG_ERR("%s: Failed to set no scan bit\n",
				__func__);
		goto exit_26100_type;
	}

	data |= 0x02;

	retval = f54->fn_ptr->write(rmi4_data,
			f54->control_base_addr,
			&data,
			sizeof(data));
	if (retval < NO_ERR) {
		TS_LOG_ERR("%s: Failed to set no scan bit\n",
				__func__);
		goto exit_26100_type;
	}
	//set all local cbc to 0
	retval = f54->fn_ptr->write(rmi4_data,f54->control.reg_96->address, &temp_data[0], f54->rmi4_data->num_of_rx);
	if (retval < NO_ERR) {
		TS_LOG_ERR("%s: fail to set all F54 control_96 registers to 0\n", __func__);
		goto exit_26100_type;
	}
	//do force update
	retval = test_do_command(COMMAND_FORCE_UPDATE);
	if (retval < NO_ERR) {
		TS_LOG_ERR("%s: Failed to do force update\n",
				__func__);
		goto exit_26100_type;
	}

	// 13. read baseline raw image, report type 100
	p_data_baseline1 = kzalloc(2 * numberOfRows * numberOfColums, GFP_KERNEL);
	if (!p_data_baseline1) {
		TS_LOG_ERR("%s: Failed to do alloc baseline1 buf\n",
				__func__);
		goto exit_26100_type;
	}
	p_data_baseline2 = kzalloc(2 * numberOfRows * numberOfColums, GFP_KERNEL);
	if (!p_data_baseline2) {
		TS_LOG_ERR("%s: Failed to do alloc baseline1 buf\n",
				__func__);
		goto exit_26100_type;
	}
	p_data_delta = kzalloc(2 * numberOfRows * numberOfColums, GFP_KERNEL);
	if (!p_data_delta) {
		TS_LOG_ERR("%s: Failed to do alloc delta buf\n",
				__func__);
		goto exit_26100_type;
	}

	command = (unsigned char)F54_TEST_100_TYPE;
	write_to_f54_register(command);
	f54->report_type = command;
	retval = synaptics_rmi4_f54_attention();
	if (retval < NO_ERR) {
		TS_LOG_ERR("Failed to read f54_read_report_image 100\n");
		goto exit_26100_type;
	}
	for (i = 0, k = 0; i < numberOfRows; i++) {
		for (j = 0; j < numberOfColums; j++) {
			temp = f54->report_data[k] | (f54->report_data[k+1] << 8);
			p_data_baseline1[i*numberOfColums+j] = temp;
			k = k + 2;
		}
	}

	for (i = 0; i< 4; i++) {

		for (ii=0; ii< ARR_RX_SIZE ; ii++) {
			minRX[ii] = 5000;
			maxRX[ii] = 0;
		}

		logical_pin = GetLogicalPin( ExtendRT26_pin[i] );

		if ( logical_pin == 0xFF )
			continue;

		TS_LOG_INFO("RT26 pin %d, logical pin %d \n", ExtendRT26_pin[i], logical_pin);


		// 14. set local CBC to 8pf(2D) 3.5pf(0D)
		temp_data[logical_pin] = 0x0f; //EXTENDED_TRX_SHORT_CBC;
		for (j=0; j< f54->rmi4_data->num_of_rx; j++)
                       TS_LOG_INFO("%s: temp_data[%d] = 0x%02x\n", __func__, j, temp_data[j]);

		retval = f54->fn_ptr->write(rmi4_data,f54->control.reg_96->address, &temp_data[0], f54->rmi4_data->num_of_rx);
		if (retval < NO_ERR) {
			TS_LOG_ERR("%s fail to set all F54 control_96 register after changing the cbc\n", __func__);
			goto exit_26100_type;
		}
		temp_data[logical_pin] = 0;

		// 15. force update
		//do force update
		retval = test_do_command(COMMAND_FORCE_UPDATE);
		if (retval < NO_ERR) {
			TS_LOG_ERR("%s: Failed to do force update\n",
					__func__);
			goto exit_26100_type;
		}

		// drop one 100 frame
		command = (unsigned char)F54_TEST_100_TYPE;
		write_to_f54_register(command);
		f54->report_type = command;
		retval = synaptics_rmi4_f54_attention();
		if (retval < NO_ERR) {
			TS_LOG_ERR("%s: fail to read f54_read_report_image 100\n", __func__);
			goto exit_26100_type;
		}
		msleep(10);

		// 16. read report type 100
		command = (unsigned char)F54_TEST_100_TYPE;
		write_to_f54_register(command);
		f54->report_type = command;
		retval = synaptics_rmi4_f54_attention();
		if (retval < NO_ERR) {
			TS_LOG_ERR("%s: fail to read f54_read_report_image 100\n", __func__);
			goto exit_26100_type;
		}

		for (ii = 0, k = 0; ii < numberOfRows; ii++) {
			for (j = 0; j < numberOfColums; j++) {
				temp = f54->report_data[k] | (f54->report_data[k+1] << 8);
				p_data_baseline2[ii*numberOfColums+j] = temp;
				k = k + 2;
			}
		}

		// 17. get delta image between baseline image 1 and baseline image 2
		for ( ii = 0; ii < f54->rmi4_data->num_of_tx; ii++ ) {
			for ( jj = 0; jj < f54->rmi4_data->num_of_rx; jj++ ) {
				p_data_delta[ii*f54->rmi4_data->num_of_rx + jj] =
						ABS( p_data_baseline1[ii*f54->rmi4_data->num_of_rx + jj], p_data_baseline2[ii*f54->rmi4_data->num_of_rx + jj] );

				if ( maxRX[jj] < p_data_delta[ii*f54->rmi4_data->num_of_rx + jj] )
					 maxRX[jj] = p_data_delta[ii*f54->rmi4_data->num_of_rx + jj];

				if ( minRX[jj] > p_data_delta[ii*f54->rmi4_data->num_of_rx + jj] )
					 minRX[jj] = p_data_delta[ii*f54->rmi4_data->num_of_rx + jj];
			}
		}

		TS_LOG_INFO("\n");
		for ( jj = 0; jj < f54->rmi4_data->num_of_rx; jj++ ) {
			TS_LOG_INFO("Rx%-2d(max, min) = (%4d, %4d)\n", jj, maxRX[jj], minRX[jj]);
		}
		TS_LOG_INFO("\n");

		// 18. Check data: TREX w/o CBC raised changes >= 200 or TREXn* (TREX with CBC raised) changes < 2000
		// Flag TRX0* as 1 as well if any other RX changes are >=200
		for ( rxIndex = 0; rxIndex < f54->rmi4_data->num_of_rx; rxIndex++ )
		{
			if ( rxIndex == logical_pin )
			{
				if ( minRX[rxIndex] < limit_1 )
				{
					TS_LOG_ERR("%s test failed: minRX[%-2d] = %4d when test pin %d (RX Logical pin [%d])\n",
							__func__, rxIndex, minRX[rxIndex], ExtendRT26_pin[i], logical_pin );
					result = EXT_TRX_SHORT_FAIL;
				}
			}
			else
			{
				if ( maxRX[rxIndex] >= limit_2 )
				{
					TS_LOG_ERR("%s test failed: maxRX[%-2d] = %4d when test pin %d (RX Logical pin [%d])\n",
							__func__, rxIndex, maxRX[rxIndex], ExtendRT26_pin[i], logical_pin );
					result = EXT_TRX_SHORT_FAIL;
				}
			}
		}
	}//for i < 4
exit_26100_type:
	if (retval < NO_ERR)
		result = EXT_TRX_SHORT_FAIL;
	if(result != EXT_TRX_SHORT_FAIL)
	{
		TS_LOG_INFO("%s: test result is succ\n", __func__);
		strncat(buf_f54test_result, SHORT_TEST_PASS, MAX_STR_LEN);
	}
	else
	{
		TS_LOG_INFO("%s: test result is fail\n", __func__);
		strncat(buf_f54test_result, SHORT_TEST_FAIL, MAX_STR_LEN);
		test_failed_reason[RAW_DATA_TYPE_OpenShort] = TEST_THRESHOLD_FAIL;
	}
	if(p_data_baseline1)
	{
		kfree(p_data_baseline1);
		p_data_baseline1 = NULL;
	}
	if(p_data_baseline2)
	{
		kfree(p_data_baseline2);
		p_data_baseline2 = NULL;
	}
	if(p_data_delta)
	{
		kfree(p_data_delta);
		p_data_delta = NULL;
	}
	if(p_data_rt26)
	{
		kfree(p_data_rt26);
		p_data_rt26 = NULL;
	}
	rmi4_data->reset_device(rmi4_data);
	return retval;
}
static int synaptics_f54_malloc(void)
{
	f54 = kzalloc(sizeof(struct synaptics_rmi4_f54_handle), GFP_KERNEL);
	if (!f54) {
		TS_LOG_ERR("Failed to alloc mem for f54\n");
		return -ENOMEM;
	}

	f54->fn_ptr =
	    kzalloc(sizeof(struct synaptics_rmi4_exp_fn_ptr), GFP_KERNEL);
	if (!f54->fn_ptr) {
		TS_LOG_ERR("Failed to alloc mem for fn_ptr\n");
		return -ENOMEM;
	}

	f54->fn55 =
	    kzalloc(sizeof(struct synaptics_rmi4_fn55_desc), GFP_KERNEL);
	if (!f54->fn55) {
		TS_LOG_ERR("Failed to alloc mem for fn55\n");
		return -ENOMEM;
	}
	f55 = kzalloc(sizeof(struct synaptics_rmi4_f55_handle), GFP_KERNEL);
	if (!f55) {
		TS_LOG_ERR("Failed to alloc mem for f55\n");
		return -ENOMEM;
	}
	return NO_ERR;
}

static void synaptics_f54_free(void)
{
	TS_LOG_INFO("kfree f54 memory\n");
	if (f54 && f54->fn_ptr)
		kfree(f54->fn_ptr);
	if (f54 && f54->fn55)
		kfree(f54->fn55);
	if (f54 && f54->mmi_buf)
		kfree(f54->mmi_buf);
	if (f54 && f54->rawdatabuf)
		kfree(f54->rawdatabuf);
	if (f54 && f54->hybridbuf)
		kfree(f54->hybridbuf);
	if (f54) {
		kfree(f54);
		f54 = NULL;
	}

	if (f55 && f55->tx_assignment)
	{
		kfree(f55->tx_assignment);
		f55->tx_assignment = NULL;
	}
	if (f55 && f55->rx_assignment)
	{
		kfree(f55->rx_assignment);
		f55->rx_assignment = NULL;
	}
	if (f55 && f55->force_tx_assignment)
	{
		kfree(f55->force_tx_assignment);
		f55->force_tx_assignment = NULL;
	}
	if (f55 && f55->force_rx_assignment)
	{
		kfree(f55->force_rx_assignment);
		f55->force_rx_assignment = NULL;
	}

	if (f55) {
		kfree(f55);
		f54 = NULL;
	}
}

static void put_capacitance_data(int index)
{
	int i = 0;
	int j = 0;
	short temp = 0;
	f54->rawdatabuf[0] = f54->rmi4_data->num_of_rx;
	f54->rawdatabuf[1] = f54->rmi4_data->num_of_tx;
	for (i = 0, j = index + 2; i < mmi_buf_size; i += 2, j++) {
		temp = (f54->mmi_buf[i]) | (f54->mmi_buf[i + 1] << 8);
		f54->rawdatabuf[j] = temp;
	}
}

static void synaptics_change_report_rate(void)
{
	int rc = NO_ERR;
	unsigned char command = 0;
	unsigned char report_rate120 = 0;
	unsigned char report_rate60 = 0;

	if (0 == f54->rmi4_data->synaptics_chip_data->report_rate_test) {
		//strncat(buf_f54test_result, "-4P", MAX_STR_LEN);
		TS_LOG_INFO("s3207 and s3706 change_report_rate default pass\n");
		return;
	}
	TS_LOG_INFO("change report rate 120 first then to 60\n");
	command = (unsigned char)F54_120HI_RATE;
	rc = f54->fn_ptr->write(f54->rmi4_data,
				f54->control_base_addr + report_rate_offset,
				&command, 1);
	if (rc < 0) {
		TS_LOG_ERR("set ic to 120HZ error because of i2c error");
		strncat(buf_f54test_result, "-4F", MAX_STR_LEN);
		return;
	}
	msleep(200);
	rc = f54->fn_ptr->read(f54->rmi4_data,
			       f54->data_base_addr + F54_READ_RATE_OFFSET,
			       &report_rate120, 1);
	if (rc < 0) {
		TS_LOG_ERR("read 120HZ from ic error because of i2c error");
		strncat(buf_f54test_result, "-4F", MAX_STR_LEN);
		return;
	}
	TS_LOG_INFO("work report_rate 120 = %d\n", report_rate120);

	command = (unsigned char)F54_60LOW_RATE;
	rc = f54->fn_ptr->write(f54->rmi4_data,
				f54->control_base_addr + report_rate_offset,
				&command, 1);
	if (rc < 0) {
		TS_LOG_ERR("write ic to 60HZ error because of i2c error");
		strncat(buf_f54test_result, "-4F", MAX_STR_LEN);
		return;
	}
	msleep(200);
	rc = f54->fn_ptr->read(f54->rmi4_data,
			       f54->data_base_addr + F54_READ_RATE_OFFSET,
			       &report_rate60, 1);
	if (rc < 0) {
		TS_LOG_ERR("read 60HZ from ic error because of i2c error");
		strncat(buf_f54test_result, "-4F", MAX_STR_LEN);
		return;
	}
	TS_LOG_INFO("work report_rate 60 = %d\n", report_rate60);

	if ((F54_DATA120_RATE == report_rate120)
	    && (F54_DATA60_RATE == report_rate60)) {
		TS_LOG_DEBUG("change rate success\n");
		strncat(buf_f54test_result, "-4P", MAX_STR_LEN);
	} else {
		TS_LOG_ERR("change rate error");
		strncat(buf_f54test_result, "-4F", MAX_STR_LEN);
		strncpy(tp_test_failed_reason, "-panel_reason",
			TP_TEST_FAILED_REASON_LEN);
		test_failed_reason[RAW_DATA_TYPE_FreShift] = TEST_THRESHOLD_FAIL;
	}
	return;
}

static int synap_strncat(char *dest, const char *src, size_t dest_sizemax, size_t src_sizemax)
{
	int rc = NO_ERR;

	if(dest == NULL || src == NULL) {
		return -EINVAL;
	}
	size_t dest_len = 0;
	size_t dest_remain_size = 0;
	dest_len = strnlen(dest, dest_sizemax);
	dest_remain_size = dest_sizemax - dest_len;

	if(src_sizemax > dest_remain_size - 1){
		return -EINVAL;
	}
	strncat(&dest[dest_len], src, src_sizemax);
	return rc;
}


static int synaptics_free_test_container(void)
{
	int rc = NO_ERR;
	if (f54->rmi4_data->synaptics_chip_data->trx_delta_test_support) {
			if (tx_delta_buf) {
					kfree(tx_delta_buf);
					tx_delta_buf = NULL;
			}
			if (rx_delta_buf) {
					kfree(rx_delta_buf);
					rx_delta_buf = NULL;
			}
	}
	if (f54->rmi4_data->synaptics_chip_data->td43xx_ee_short_test_support) {
			if (g_td43xx_rt95_part_one) {
					kfree(g_td43xx_rt95_part_one);
					g_td43xx_rt95_part_one = NULL;
			}
			if (g_td43xx_rt95_part_two) {
					kfree(g_td43xx_rt95_part_two);
					g_td43xx_rt95_part_two = NULL;
			}
	}

	return rc;
}
static int synaptics_alloc_test_container(void)
{
	int rc = NO_ERR;
	if (f54->rmi4_data->synaptics_chip_data->trx_delta_test_support) {
			tx_delta_buf = (int *)kzalloc(mmi_buf_size * sizeof(int), GFP_KERNEL);
			if (!tx_delta_buf) {
				TS_LOG_ERR("malloc tx_delta_buf failed\n");
				goto out;
			}
			rx_delta_buf = (int *)kzalloc(mmi_buf_size * sizeof(int), GFP_KERNEL);
			if (!rx_delta_buf) {
				TS_LOG_ERR("malloc rx_delta_buf failed\n");
				goto out;
			}
	}

	if (f54->rmi4_data->synaptics_chip_data->td43xx_ee_short_test_support) {
			g_td43xx_rt95_part_one = (signed int *)kzalloc(mmi_buf_size * sizeof(int), GFP_KERNEL);
			if (!g_td43xx_rt95_part_one) {
				TS_LOG_ERR("malloc g_td43xx_rt95_part_one failed\n");
				goto out;
			}
			g_td43xx_rt95_part_two = (signed int *)kzalloc(mmi_buf_size * sizeof(int), GFP_KERNEL);
			if (!g_td43xx_rt95_part_two) {
				TS_LOG_ERR("malloc g_td43xx_rt95_part_two failed\n");
				goto out;
			}
	}
	return rc;
out:
	synaptics_free_test_container();
	return -ENOMEM;
}

static int synaptics_alloc_newnode(struct ts_rawdata_newnodeinfo ** pts_node,int rawbufsize)
{
	int rc = NO_ERR;
	*pts_node = (struct ts_rawdata_newnodeinfo *)kzalloc(sizeof(struct ts_rawdata_newnodeinfo), GFP_KERNEL);
	if (!(*pts_node)) {
		TS_LOG_ERR("malloc failed \n");
		return -EINVAL;
	}
	(*pts_node)->values = kzalloc((rawbufsize)*sizeof(int), GFP_KERNEL);
	if (!(*pts_node)->values) {
		TS_LOG_ERR("malloc failed  for values \n");
		kfree(*pts_node);
		*pts_node = NULL;
		return -EINVAL;
	}
	return rc;
}
static void synap_put_cap_test_deviceinfo(struct ts_rawdata_info_new *info)
{
	size_t pjt_sizemax = sizeof(f54->rmi4_data->rmi4_mod_info.project_id_string);
	size_t ver_sizemax = sizeof(f54->rmi4_data->synaptics_chip_data->version_name);
	size_t product_sizemax = sizeof(f54->rmi4_data->rmi4_mod_info.product_id_string);
	size_t devinfo_size = sizeof(info->deviceinfo);
	synap_strncat(info->deviceinfo, "synaptics-", devinfo_size, sizeof("synaptics-"));
	synap_strncat(info->deviceinfo,f54->rmi4_data->rmi4_mod_info.product_id_string,
		devinfo_size,product_sizemax);
	synap_strncat(info->deviceinfo, "-", devinfo_size,sizeof("-"));
	synap_strncat(info->deviceinfo,f54->rmi4_data->rmi4_mod_info.project_id_string,
		devinfo_size,pjt_sizemax);
	synap_strncat(info->deviceinfo, "-", devinfo_size,sizeof("-"));
	synap_strncat(info->deviceinfo,f54->rmi4_data->synaptics_chip_data->version_name,
		devinfo_size,ver_sizemax);
	synap_strncat(info->deviceinfo, ";", devinfo_size,sizeof(";"));
	return;
}
int synap_put_cap_test_dataNewformat(struct ts_rawdata_info_new *info)
{
	int rc = NO_ERR;
	int test_num = 1;
	int test_tmp_num = 3;
	int rawbufsize = f54->rmi4_data->num_of_tx * f54->rmi4_data->num_of_rx;
	int rawSelfbuf = f54->rmi4_data->num_of_tx + f54->rmi4_data->num_of_rx;
	int i = 0;
	struct ts_rawdata_newnodeinfo * pts_node = NULL;
	char statistics_tmp_data[STATISTICS_DATA_LEN] = {0};
	TS_LOG_INFO("synap_put_cap_test_dataNewformat called\n");

	info->tx = f54->rmi4_data->num_of_tx;
	info->rx = f54->rmi4_data->num_of_rx;
	if (f54->rmi4_data->synaptics_chip_data->rawdata_arrange_type == TS_RAWDATA_TRANS_ABCD2CBAD
		|| f54->rmi4_data->synaptics_chip_data->rawdata_arrange_type == TS_RAWDATA_TRANS_ABCD2ADCB) {
		ts_kit_rotate_rawdata_abcd2cbad(info->rx, info->tx, f54->rawdatabuf + 2,
				f54->rmi4_data->synaptics_chip_data->rawdata_arrange_type);
		ts_kit_rotate_rawdata_abcd2cbad(info->rx, info->tx,
				f54->rawdatabuf + rawbufsize + 2, f54->rmi4_data->synaptics_chip_data->rawdata_arrange_type);
	}
	/*deviceinfo*/
	synap_put_cap_test_deviceinfo(info);
	/*i2c info */
	if(strncmp(buf_f54test_result, "0P", strlen("0P"))) {
		synap_strncat(info->i2cinfo, "0F", sizeof(info->i2cinfo),sizeof("0F"));
		synap_strncat(info->i2cerrinfo, "software reason ", sizeof(info->i2cerrinfo),sizeof("software reason "));
		return rc;
	} else {
		synap_strncat(info->i2cinfo, "0P", sizeof(info->i2cinfo),sizeof("0P"));
	}
	test_num = test_num + test_tmp_num;
	/*rawdata*/
	if(f54->rmi4_data->synaptics_chip_data->test_capacitance_via_csvfile
		|| f54->rmi4_data->synaptics_chip_data->test_enhance_raw_data_capacitance) {
		rc = synaptics_alloc_newnode(&pts_node, rawbufsize);
		if(rc < 0) {
			TS_LOG_ERR("malloc failed 1pts_node\n");
			return rc;
		}
		snprintf(statistics_tmp_data, STATISTICS_DATA_LEN - 1,
			 "[%4d,%4d,%4d]",
			 f54->raw_statics_data.RawimageAverage,
			 f54->raw_statics_data.RawimageMaxNum,
			 f54->raw_statics_data.RawimageMinNum);

		for (i = 0; i < rawbufsize; i++)
			pts_node->values[i] = f54->rawdatabuf[i+2];
		strncpy(pts_node->statistics_data,statistics_tmp_data,sizeof(pts_node->statistics_data)-1);
		memset(statistics_tmp_data, 0, sizeof(statistics_tmp_data));
		pts_node->size = rawbufsize;
		pts_node->testresult = buf_f54test_result[test_num];
		pts_node->typeindex = RAW_DATA_TYPE_CAPRAWDATA;
		strncpy(pts_node->test_name,"Cap_Rawdata",sizeof(pts_node->test_name)-1);
		list_add_tail(&pts_node->node, &info->rawdata_head);
		test_num = test_num + test_tmp_num;

		/* tp test failed reason */
		if (0 == strlen(buf_f54test_result) || strstr(buf_f54test_result, "F")) {
			strncpy(pts_node->tptestfailedreason, "software reason", TS_RAWDATA_FAILED_REASON_LEN);
			for(i = 0; i < RAW_DATA_END; i++) {
				if(TEST_THRESHOLD_FAIL == test_failed_reason[i]) {
					strncpy(pts_node->tptestfailedreason, "-panel_reason", TS_RAWDATA_FAILED_REASON_LEN);
				}
			}
		}

		/*Trx delta*/
		rc = synaptics_alloc_newnode(&pts_node, mmi_buf_size);
		if(rc < 0) {
				TS_LOG_ERR("malloc failed 2pts_node\n");
				return rc;
		}
		if (f54->rmi4_data->synaptics_chip_data->trx_delta_test_support) {
			for (i = 0; i < rawbufsize; i++)
					pts_node->values[i] =rx_delta_buf[i];

			for(i= 0; i < rawbufsize; i++)
					pts_node->values[i+rawbufsize] =tx_delta_buf[i];

			pts_node->size = mmi_buf_size;
		}
		pts_node->testresult = buf_f54test_result[test_num];
		pts_node->typeindex = RAW_DATA_TYPE_TrxDelta;
		strncpy(pts_node->test_name,"Trx delta",sizeof(pts_node->test_name)-1);
		list_add_tail(&pts_node->node, &info->rawdata_head);
		test_num = test_num + test_tmp_num;
	}
	/*3p3f*/
	rc = synaptics_alloc_newnode(&pts_node, rawbufsize);
	if(rc < 0) {
		TS_LOG_ERR("malloc failed 3pts_node\n");
		return rc;
	}
	snprintf((statistics_tmp_data ), STATISTICS_DATA_LEN - 1,
		 "[%4d,%4d,%4d]",
		 f54->delta_statics_data.RawimageAverage,
		 f54->delta_statics_data.RawimageMaxNum,
		 f54->delta_statics_data.RawimageMinNum);
	for (i = 0; i < rawbufsize; i++)
			pts_node->values[i] = f54->rawdatabuf[rawbufsize+2+i];
	pts_node->size = rawbufsize;
	pts_node->testresult = buf_f54test_result[test_num];
	pts_node->typeindex = RAW_DATA_TYPE_Noise;
	strncpy(pts_node->statistics_data,statistics_tmp_data,sizeof(pts_node->statistics_data)-1);
	memset(statistics_tmp_data, 0, sizeof(statistics_tmp_data));
	strncpy(pts_node->test_name,"noise delta",sizeof(pts_node->test_name)-1);
	list_add_tail(&pts_node->node, &info->rawdata_head);
	test_num = test_num + test_tmp_num;

	/*4p4f*/
	if(f54->rmi4_data->synaptics_chip_data->report_rate_test) {
		pts_node = (struct ts_rawdata_newnodeinfo *)kzalloc(sizeof(struct ts_rawdata_newnodeinfo), GFP_KERNEL);
		if (!pts_node) {
			TS_LOG_ERR("malloc failed 4pts_node\n");
			return -EINVAL;
		}
		pts_node->testresult = buf_f54test_result[test_num];
		pts_node->typeindex = RAW_DATA_TYPE_FreShift;
		strncpy(pts_node->test_name,"Frequency shift",sizeof(pts_node->test_name)-1);
		list_add_tail(&pts_node->node, &info->rawdata_head);
		test_num = test_num + test_tmp_num;
	}

	/*5p5f*/
	if(f54->rmi4_data->synaptics_chip_data->td43xx_ee_short_test_support
			|| f54->rmi4_data->synaptics_chip_data->support_s3320_short_test
			|| f54->rmi4_data->synaptics_chip_data->support_ext_trex_short_test) {
		pts_node = (struct ts_rawdata_newnodeinfo *)kzalloc(sizeof(struct ts_rawdata_newnodeinfo), GFP_KERNEL);
		if (!pts_node) {
			TS_LOG_ERR("malloc failed 5pts_node\n");
			return -EINVAL;
		}
		pts_node->testresult = buf_f54test_result[test_num];
		pts_node->typeindex = RAW_DATA_TYPE_OpenShort;
		strncpy(pts_node->test_name,"Open short",sizeof(pts_node->test_name)-1);
		list_add_tail(&pts_node->node, &info->rawdata_head);
		test_num = test_num + test_tmp_num;
	}

	/*6p6f*/
	if(f54->rmi4_data->synaptics_chip_data->self_cap_test) {
		rc = synaptics_alloc_newnode(&pts_node, rawSelfbuf);
		if(rc < 0) {
			TS_LOG_ERR("malloc failed 6pts_node\n");
			return rc;
		}
		pts_node->testresult = buf_f54test_result[test_num];
		pts_node->typeindex = RAW_DATA_TYPE_SelfCap;
		pts_node->size = rawSelfbuf;
		for (i = 0; i < rawSelfbuf; i++)
			pts_node->values[i] =f54->hybridbuf[i+mmi_hybrid_abs_delta /4];
		strncpy(pts_node->test_name,"Self Cap",sizeof(pts_node->test_name)-1);
		list_add_tail(&pts_node->node, &info->rawdata_head);
		test_num = test_num + test_tmp_num;

		/*9p9f*/
		rc = synaptics_alloc_newnode(&pts_node, rawSelfbuf);
		if(rc < 0) {
			TS_LOG_ERR("malloc failed 9pts_node\n");
			return rc;
		}
		pts_node->testresult = buf_f54test_result[test_num];
		pts_node->typeindex = RAW_DATA_TYPE_SelfNoisetest;
		pts_node->size = rawSelfbuf;
		for (i = 0; i < rawSelfbuf; i++)
			pts_node->values[i] =f54->hybridbuf[i];
		strncpy(pts_node->test_name,"Self Noise",sizeof(pts_node->test_name)-1);
		list_add_tail(&pts_node->node, &info->rawdata_head);
		test_num = test_num + test_tmp_num;
	}

	return rc;
}

int synap_get_cap_data(struct ts_rawdata_info *info)
{
	int rc = NO_ERR;
	unsigned char command = 0;

	TS_LOG_INFO("synap_get_cap_data called\n");
	if(f54->rmi4_data->synaptics_chip_data->rawdata_newformatflag == TS_RAWDATA_NEWFORMAT) {
		rc = synaptics_alloc_test_container();
		if(rc < 0) {
			TS_LOG_ERR("%s synaptics_alloc_test_container  failed ! \n", __func__);
			goto exit;
		}

	} else {
		tx_delta_buf = info->tx_delta_buf;
		rx_delta_buf = info->rx_delta_buf;
		g_td43xx_rt95_part_one = info->td43xx_rt95_part_one;
		g_td43xx_rt95_part_two = info->td43xx_rt95_part_two;
	}
	memset(test_failed_reason, 0, sizeof(test_failed_reason));
	memset(buf_f54test_result, 0, sizeof(buf_f54test_result));
	memset(f54->rawdatabuf, 0, rawdata_size * sizeof(int));
	memset(f54->hybridbuf, 0 , HYBRID_BUF_SIZE * sizeof(u32));
	if (SYNAPTICS_TD4322 != f54->rmi4_data->synaptics_chip_data->ic_type
	&&SYNAPTICS_TD4310 != f54->rmi4_data->synaptics_chip_data->ic_type) {
		rc = f54->rmi4_data->status_save(f54->rmi4_data);
		if (rc < 0) {
			TS_LOG_ERR
				("failed to save glove/holster/palm or other status!\n");
		}
	}
	rc = f54->fn_ptr->read(f54->rmi4_data, f54->data_base_addr, &command,
			       1);
	if (rc < 0) {		/*i2c communication failed, mmi result is all failed */
		memcpy(buf_f54test_result, "0F-1F-2F",
		       (strlen("0F-1F-2F") + 1));
	} else {
		memcpy(buf_f54test_result, "0P-", (strlen("0P-") + 1));

		if (f54->rmi4_data->synaptics_chip_data->test_enhance_raw_data_capacitance) {
			mmi_enhance_raw_capacitance_test();	/*report type == 3 */
		} else if (f54->rmi4_data->synaptics_chip_data->test_capacitance_via_csvfile) {
			mmi_csvfile_rawdata_test();
		} else {
			TS_LOG_ERR("[%s] devkit  not support dts raw threshold \n",__func__);
			rc = -EINVAL;
			goto exit;
			//mmi_rawcapacitance_test();	/*1-2p/f*/
		}

		TS_LOG_INFO ("Mutul rawdata test!!!\n");
		put_capacitance_data(0);
		mmi_deltacapacitance_test();	/*3p3f*/
		synaptics_change_report_rate(); /*4p4f*/
		if (f54->rmi4_data->synaptics_chip_data->td43xx_ee_short_test_support) {
			mmi_td43xx_ee_short_report();	/*report type == 95 */
		}
		put_capacitance_data(mmi_buf_size / 2);

		if (f54->rmi4_data->synaptics_chip_data->support_s3320_short_test){
			mmi_trex_shorts_test();
		}
		if(f54->rmi4_data->synaptics_chip_data->support_ext_trex_short_test)
		{
			mmi_ext_trx_short_test();
		}

		if(f54->rmi4_data->synaptics_chip_data->self_cap_test)
		{
			TS_LOG_INFO ("Self rawdata test!!!%d \n", f54->rmi4_data->synaptics_chip_data->self_cap_test);
			TS_LOG_INFO ("%s: test_rawdata_normalizing = %x\n", __func__,
			     f54->rmi4_data->synaptics_chip_data->test_rawdata_normalizing);
			TS_LOG_INFO("seltest----start~!\n");
			mmi_hybrid_raw_cap_test();
			mmi_hybrid_abs_delta_test();
			TS_LOG_INFO("seltest----end~!\n");
		}
		mmi_add_static_data();
	}
	if ((SYNAPTICS_TD4322 != f54->rmi4_data->synaptics_chip_data->ic_type
		&& SYNAPTICS_TD4310 != f54->rmi4_data->synaptics_chip_data->ic_type)
		|| f54->rmi4_data->synaptics_chip_data->td43xx_ee_short_test_support) {
		rc = f54->rmi4_data->reset_device(f54->rmi4_data);
		if (rc < 0) {
			TS_LOG_ERR("failed to write command to f01 reset!\n");
			goto exit;
		}
		rc = f54->rmi4_data->status_resume(f54->rmi4_data);
		if (rc < 0) {
			TS_LOG_ERR
				("failed to resume glove/holster/palm or other status!\n");
		}
	}
	if(f54->rmi4_data->synaptics_chip_data->rawdata_newformatflag == TS_RAWDATA_NEWFORMAT) {
		synap_put_cap_test_dataNewformat((struct ts_rawdata_info_new *)info);
		goto exit;
	}

	memcpy(info->buff, f54->rawdatabuf, rawdata_size * sizeof(int));

	info->hybrid_buff[0] = f54->rmi4_data->num_of_rx;
	info->hybrid_buff[1] = f54->rmi4_data->num_of_tx;
	memcpy(&info->hybrid_buff[2], f54->hybridbuf, (mmi_hybrid_abs_delta/2) * sizeof(int));

	strncat(buf_f54test_result, ";", strlen(";"));
	if (0 == strlen(buf_f54test_result) || strstr(buf_f54test_result, "F")) {
		strncat(buf_f54test_result, tp_test_failed_reason,
			strlen(tp_test_failed_reason));
	}

	switch (f54->rmi4_data->synaptics_chip_data->ic_type) {
	case SYNAPTICS_S3207:
		strncat(buf_f54test_result, "-synaptics_3207",
			strlen("-synaptics_3207"));
		break;
	case SYNAPTICS_S3350:
		strncat(buf_f54test_result, "-synaptics_3350",
			strlen("-synaptics_3350"));
		break;
	case SYNAPTICS_S3320:
		strncat(buf_f54test_result, "-synaptics_3320",
			strlen("-synaptics_3320"));
		break;
	case SYNAPTICS_S3706:
		strncat(buf_f54test_result, "-synaptics_3706",
			strlen("-synaptics_3706"));
		break;
	case SYNAPTICS_S3718:
	case SYNAPTICS_TD4322:
	case SYNAPTICS_TD4310:
		strncat(buf_f54test_result, "-synaptics_",
			strlen("-synaptics_"));
		strncat(buf_f54test_result,
			f54->rmi4_data->rmi4_mod_info.project_id_string,
			strlen(f54->rmi4_data->rmi4_mod_info.
			       project_id_string));
		break;
	default:
		TS_LOG_ERR("failed to recognize ic_ver\n");
		break;
	}

	memcpy(info->result, buf_f54test_result, strlen(buf_f54test_result));
	info->used_size = rawdata_size;
	info->used_synaptics_self_cap_size = mmi_hybrid_abs_delta >> 1;
	TS_LOG_INFO("info->used_size = %d\n", info->used_size);
	rc = NO_ERR;
exit:
	if(f54->rmi4_data->synaptics_chip_data->rawdata_newformatflag == TS_RAWDATA_NEWFORMAT) {
		synaptics_free_test_container();
	}
	synaptics_f54_free();
	return rc;
}

static int synaptics_rmi4_f54_attention_cust(void)
{
	int retval = 0;
	int l = 0;
	unsigned char report_index[2];
	int i = 0;
	unsigned int report_times_max = 0;
	unsigned int report_size_temp = MAX_I2C_MSG_LENS;
	unsigned char *report_data_temp = NULL;

	set_report_size();

	if (f54->report_size == 0) {
		TS_LOG_ERR("Report data size = 0\n");
		retval = -EINVAL;
		goto error_exit;
	}

	if (f54->data_buffer_size < f54->report_size){
		if ((f54->data_buffer_size) && (f54->report_data)) {
			kfree(f54->report_data);
			f54->report_data = NULL;
		}
		f54->report_data = kzalloc(f54->report_size, GFP_KERNEL);
		if (!f54->report_data) {
			TS_LOG_ERR("Failed to alloc mem for data buffer\n");
			f54->data_buffer_size = 0;
			retval = -ENOMEM;
			goto error_exit;
		}
		f54->data_buffer_size = f54->report_size;
	}

	report_times_max = f54->report_size/MAX_I2C_MSG_LENS;
	if(f54->report_size%MAX_I2C_MSG_LENS != 0)
	{
		report_times_max += 1;
	}

	report_index[0] = 0;
	report_index[1] = 0;

	retval = f54->fn_ptr->write(f54->rmi4_data,
			f54->data_base_addr + DATA_REPORT_INDEX_OFFSET,
			report_index,
			sizeof(report_index));
	if (retval < 0) {
		TS_LOG_ERR("Failed to write report data index\n");
		retval = -EINVAL;
		goto error_exit;
	}

	/* Point to the block data about to transfer */
	report_data_temp = f54->report_data;
	TS_LOG_INFO("report_size = %d.\n",f54->report_size);
	TS_LOG_INFO("report_times_max = %d.\n",report_times_max);

	for(i = 0;i < report_times_max;i ++){
		if(i == (report_times_max - 1))
		{
			/* The last time transfer the rest of the block data */
			report_size_temp = f54->report_size%MAX_I2C_MSG_LENS;
			/* Bug:if f54->report_size % MAX_I2C_MSG_LENS == 0,
			the last time transfer data len is MAX_I2C_MSG_LENS.
			*/
			report_size_temp = (report_size_temp != 0) ? report_size_temp : MAX_I2C_MSG_LENS;
		}
		TS_LOG_DEBUG("i = %d,report_size_temp = %d.\n",i,report_size_temp);
		retval = f54->fn_ptr->read(f54->rmi4_data,
			f54->data_base_addr + DATA_REPORT_DATA_OFFSET,
			report_data_temp,
			report_size_temp);
		if (retval < 0) {
			TS_LOG_ERR("%s: Failed to read report data\n",__func__);
			retval = -EINVAL;
			//mutex_unlock(&f54->data_mutex);
			goto error_exit;
		}
		/* Point to the next 256bytes data */
		report_data_temp += MAX_I2C_MSG_LENS;
	}

	retval = NO_ERR;

error_exit:
	return retval;
}

int synap_get_calib_data(struct ts_calibration_data_info *info)
{
	int rc = NO_ERR;
	unsigned char command = 0;

	TS_LOG_INFO("%s called\n", __FUNCTION__);

	command = (unsigned char) F54_CALIBRATION;

	write_to_f54_register(command);
	f54->report_type = command;
	rc = synaptics_rmi4_f54_attention_cust();
	if(rc < 0){
		TS_LOG_ERR("Failed to get data\n");
		goto exit;
	}

	memcpy(info->data, f54->report_data, f54->report_size);

	info->used_size = f54->report_size;
	TS_LOG_INFO("info->used_size = %d\n", info->used_size);
	info->tx_num = f54->rmi4_data->num_of_tx;
	info->rx_num = f54->rmi4_data->num_of_rx;
	TS_LOG_INFO("info->tx_num = %d\n", info->tx_num);
	TS_LOG_INFO("info->rx_num = %d\n", info->rx_num);
	rc = NO_ERR;
exit:
	synaptics_f54_free();
	return rc;
}

int synap_get_calib_info(struct ts_calibration_info_param *info)
{
	int rc = NO_ERR;
	char calibration_state = 0;

	TS_LOG_INFO("%s called\n", __FUNCTION__);
	TS_LOG_INFO("calibration info reg offset: 0x%x\n", CALIBRATION_INFO_OFFSET);

	rc = f54->fn_ptr->read(f54->rmi4_data,
		f54->data_base_addr + CALIBRATION_INFO_OFFSET,
		&calibration_state,
		sizeof (calibration_state));
	if(rc < 0){
		TS_LOG_ERR("Failed to get calibration info\n");
		goto exit;
	}
	TS_LOG_INFO("calibration_state = 0x%x\n", calibration_state);

	info->calibration_crc = ((calibration_state >> 1) & 1);
	TS_LOG_INFO("info->calibration_crc = %d\n", info->calibration_crc);
	rc = NO_ERR;
exit:
	synaptics_f54_free();
	return rc;
}

int synap_debug_data_test(struct ts_diff_data_info *info)
{
	int rc = NO_ERR;

	TS_LOG_INFO("synaptics_get_debug_cap_data called\n");

	memset(buf_f54test_result, 0, sizeof(buf_f54test_result));
	memset(f54->rawdatabuf, 0, rawdata_size * sizeof(int));
	memset(f54->mmi_buf, 0, mmi_buf_size);

	switch (info->debug_type) {
	case READ_DIFF_DATA:
		mmi_deltacapacitance_test();	/*report type == 2 */
		put_capacitance_data(0);
		break;
	case READ_RAW_DATA:
		mmi_rawcapacitance_test();	/*report type == 3 */
		put_capacitance_data(0);
		break;
	default:
		TS_LOG_ERR("failed to recognize ic_ver\n");
		break;
	}

	memcpy(info->buff, f54->rawdatabuf, rawdata_size * sizeof(int));
	memcpy(info->result, buf_f54test_result, strlen(buf_f54test_result));
	info->used_size = rawdata_size / 2;

	synaptics_f54_free();
	return rc;
}

static int synaptics_rmi4_f54_attention(void)
{
	int retval = 0;
	int l = 0;
	unsigned char report_index[2] = {0};
	int i = 0;
	unsigned int report_times_max = 0;
	unsigned int report_size_temp = MAX_I2C_MSG_LENS;
	unsigned char *report_data_temp = NULL;

	set_report_size();

	if (f54->report_size == 0) {
		TS_LOG_ERR("Report data size = 0\n");
		retval = -EINVAL;
		goto error_exit;
	}

	if (f54->data_buffer_size < f54->report_size) {
		if ((f54->data_buffer_size) && (f54->report_data)) {
			kfree(f54->report_data);
			f54->report_data = NULL;
		}
		f54->report_data = kzalloc(f54->report_size, GFP_KERNEL);
		if (!f54->report_data) {
			TS_LOG_ERR("Failed to alloc mem for data buffer\n");
			f54->data_buffer_size = 0;
			retval = -ENOMEM;
			goto error_exit;
		}
		f54->data_buffer_size = f54->report_size;
	}

	report_times_max = f54->report_size / MAX_I2C_MSG_LENS;
	if (f54->report_size % MAX_I2C_MSG_LENS != 0) {
		report_times_max += 1;
	}

	report_index[0] = 0;
	report_index[1] = 0;

	retval = f54->fn_ptr->write(f54->rmi4_data,
				    f54->data_base_addr +
				    DATA_REPORT_INDEX_OFFSET, report_index,
				    sizeof(report_index));
	if (retval < 0) {
		TS_LOG_ERR("Failed to write report data index\n");
		retval = -EINVAL;
		goto error_exit;
	}

	/* Point to the block data about to transfer */
	report_data_temp = f54->report_data;
	TS_LOG_INFO("report_size = %d.\n", f54->report_size);
	TS_LOG_INFO("report_times_max = %d.\n", report_times_max);

	for (i = 0; i < report_times_max; i++) {
		if (i == (report_times_max - 1)) {
			/* The last time transfer the rest of the block data */
			report_size_temp = f54->report_size % MAX_I2C_MSG_LENS;
			/* Bug:if f54->report_size % MAX_I2C_MSG_LENS == 0,
			   the last time transfer data len is MAX_I2C_MSG_LENS.
			 */
			report_size_temp =
			    (report_size_temp !=
			     0) ? report_size_temp : MAX_I2C_MSG_LENS;
		}
		TS_LOG_DEBUG("i = %d,report_size_temp = %d.\n", i,
			     report_size_temp);
		retval =
		    f54->fn_ptr->read(f54->rmi4_data,
				      f54->data_base_addr +
				      DATA_REPORT_DATA_OFFSET, report_data_temp,
				      report_size_temp);
		if (retval < 0) {
			TS_LOG_ERR("%s: Failed to read report data\n",
				   __func__);
			retval = -EINVAL;
			/*mutex_unlock(&f54->data_mutex);*/
			goto error_exit;
		}
		/* Point to the next 256bytes data */
		report_data_temp += MAX_I2C_MSG_LENS;
	}

	if (f54->report_size > mmi_buf_size)
		return NO_ERR;
	/*get report data, one data contains two bytes */
	for (l = 0; l < f54->report_size; l += 2) {
		f54->mmi_buf[l] = f54->report_data[l];
		f54->mmi_buf[l + 1] = f54->report_data[l + 1];
	}

	retval = NO_ERR;

error_exit:
	return retval;
}

static int synaptics_read_f34(void)
{
	int retval = NO_ERR;

	if (SYNAPTICS_S3718 != f54->rmi4_data->synaptics_chip_data->ic_type) {
		retval = f54->fn_ptr->read(f54->rmi4_data,
					   f54->f34_fd.query_base_addr +
					   BOOTLOADER_ID_OFFSET,
					   f54->bootloader_id,
					   sizeof(f54->bootloader_id));
		if (retval < 0) {
			TS_LOG_ERR("Failed to read bootloader ID\n");
			return retval;
		}
	} else {
		retval = f54->fn_ptr->read(f54->rmi4_data,
					   f54->f34_fd.query_base_addr +
					   BOOTLOADER_ID_OFFSET_V7,
					   f54->bootloader_id,
					   sizeof(f54->bootloader_id));
		if (retval < 0) {
			TS_LOG_ERR("Failed to read bootloader ID\n");
			return retval;
		}
	}
	/*V5 V6 version is char data, as '5' '6', V7 is int data, 7 */
	TS_LOG_INFO("bootloader_id[1] = %c, %d\n", f54->bootloader_id[1],
		    f54->bootloader_id[1]);

	switch (f54->bootloader_id[1]) {
	case '5':
		f54->bl_version = V5;
		break;
	case '6':
		f54->bl_version = V6;
		break;
	case 7:
		f54->bl_version = V7;
		break;
	default:
		TS_LOG_ERR("unknown %d %c\n", f54->bootloader_id[1],
			   f54->bootloader_id[1]);
		break;
	}

	if (SYNAPTICS_S3207 != f54->rmi4_data->synaptics_chip_data->ic_type) {
		if (V5 == f54->bl_version) {
			/*get tx and rx value by read register from F11_2D_CTRL77 and F11_2D_CTRL78 */
			retval =
			    f54->fn_ptr->read(f54->rmi4_data,
					      f54->rmi4_data->rmi4_feature.
					      f01_ctrl_base_addr + RX_NUMBER,
					      &f54->rmi4_data->num_of_rx, 1);
			if (retval < 0) {
				TS_LOG_ERR
				    ("Could not read RX value from 0x%04x\n",
				     f54->rmi4_data->rmi4_feature.
				     f01_ctrl_base_addr + RX_NUMBER);
				return -EINVAL;
			}

			retval =
			    f54->fn_ptr->read(f54->rmi4_data,
					      f54->rmi4_data->rmi4_feature.
					      f01_ctrl_base_addr + TX_NUMBER,
					      &f54->rmi4_data->num_of_tx, 1);
			if (retval < 0) {
				TS_LOG_ERR
				    ("Could not read TX value from 0x%04x\n",
				     f54->rmi4_data->rmi4_feature.
				     f01_ctrl_base_addr + TX_NUMBER);
				return -EINVAL;
			}
		}
	} else {
		retval =
		    f54->fn_ptr->read(f54->rmi4_data,
				      f54->rmi4_data->rmi4_feature.
				      f11_ctrl_base_addr + RX_NUMBER_S3207,
				      &f54->rmi4_data->num_of_rx, 1);
		if (retval < 0) {
			TS_LOG_ERR("Could not read RX value from 0x%04x\n",
				   f54->rmi4_data->rmi4_feature.
				   f11_ctrl_base_addr + RX_NUMBER_S3207);
			return -EINVAL;
		}

		retval =
		    f54->fn_ptr->read(f54->rmi4_data,
				      f54->rmi4_data->rmi4_feature.
				      f11_ctrl_base_addr + TX_NUMBER_S3207,
				      &f54->rmi4_data->num_of_tx, 1);
		if (retval < 0) {
			TS_LOG_ERR("Could not read TX value from 0x%04x\n",
				   f54->rmi4_data->rmi4_feature.
				   f11_ctrl_base_addr + TX_NUMBER_S3207);
			return -EINVAL;
		}
	}

	/*used for force touch data. */
	if (1 == f54->rmi4_data->synaptics_chip_data->support_3d_func) {
		TS_LOG_INFO("support 3d test\n");
		f54->rmi4_data->num_of_tx =
		    (f54->rmi4_data->num_of_tx) - FORCE_NUMER;
	}
	rawdata_size =
	    (f54->rmi4_data->num_of_tx) * (f54->rmi4_data->num_of_rx) * 2 + 2;
	mmi_buf_size =
	    (f54->rmi4_data->num_of_tx) * (f54->rmi4_data->num_of_rx) * 2;
	if(rawdata_size <= 0 || mmi_buf_size <= 0) {
		TS_LOG_INFO("%s error rawdata_size = %d, mmi_buf_size = %d\n" ,rawdata_size, mmi_buf_size);
		return -EINVAL;
	}

	TS_LOG_INFO("rx = %d, tx = %d, rawdata_size = %d, mmi_buf_size = %d\n",
		    f54->rmi4_data->num_of_rx, f54->rmi4_data->num_of_tx,
		    rawdata_size, mmi_buf_size);
	return NO_ERR;
}

static void Synaptics_test_free_control_mem(void)
{
	struct f54_control control = f54->control;

	kfree(control.reg_7);
	control.reg_7 = NULL;
	kfree(control.reg_41);
	control.reg_41 = NULL;
	kfree(control.reg_57);
	control.reg_57 = NULL;
	kfree(control.reg_86);
	control.reg_86 = NULL;
	kfree(control.reg_88);
	control.reg_88 = NULL;
	kfree(control.reg_96);
	control.reg_96 = NULL;
	kfree(control.reg_110);
	control.reg_110 = NULL;
	kfree(control.reg_149);
	control.reg_149 = NULL;
	kfree(control.reg_188);
	control.reg_188 = NULL;

	return;
}

static int Synaptics_test_set_controls(void)
{
	unsigned char length = 0;
	unsigned char num_of_sensing_freqs =0;
	unsigned short reg_addr = f54->control_base_addr;
	struct f54_control *control = &f54->control;

	num_of_sensing_freqs = f54->query.number_of_sensing_frequencies;

	/* control 0 */
	reg_addr += CONTROL_0_SIZE;

	/* control 1 */
	if ((f54->query.touch_controller_family == 0) ||
	    (f54->query.touch_controller_family == 1))
		reg_addr += CONTROL_1_SIZE;

	/* control 2 */
	reg_addr += CONTROL_2_SIZE;

	/* control 3 */
	if (f54->query.has_pixel_touch_threshold_adjustment == 1)
		reg_addr += CONTROL_3_SIZE;

	/* controls 4 5 6 */
	if ((f54->query.touch_controller_family == 0) ||
	    (f54->query.touch_controller_family == 1))
		reg_addr += CONTROL_4_6_SIZE;

	/* control 7 */
	if (f54->query.touch_controller_family == 1) {
		control->reg_7 = kzalloc(sizeof(*(control->reg_7)), GFP_KERNEL);
		if (!control->reg_7)
			goto exit_no_mem;
		control->reg_7->address = reg_addr;
		reg_addr += CONTROL_7_SIZE;
	}

	/* controls 8 9 */
	if ((f54->query.touch_controller_family == 0) ||
	    (f54->query.touch_controller_family == 1))
		reg_addr += CONTROL_8_9_SIZE;

	/* control 10 */
	if (f54->query.has_interference_metric == 1)
		reg_addr += CONTROL_10_SIZE;

	/* control 11 */
	if (f54->query.has_ctrl11 == 1)
		reg_addr += CONTROL_11_SIZE;

	/* controls 12 13 */
	if (f54->query.has_relaxation_control == 1)
		reg_addr += CONTROL_12_13_SIZE;

	/* controls 14 15 16 */
	if (f54->query.has_sensor_assignment == 1) {
		reg_addr += CONTROL_14_SIZE;
		reg_addr += CONTROL_15_SIZE * f54->query.num_of_rx_electrodes;
		reg_addr += CONTROL_16_SIZE * f54->query.num_of_tx_electrodes;
	}

	/* controls 17 18 19 */
	if (f54->query.has_sense_frequency_control == 1) {
		reg_addr += CONTROL_17_SIZE * num_of_sensing_freqs;
		reg_addr += CONTROL_18_SIZE * num_of_sensing_freqs;
		reg_addr += CONTROL_19_SIZE * num_of_sensing_freqs;
	}

	/* control 20 */
	reg_addr += CONTROL_20_SIZE;

	/* control 21 */
	if (f54->query.has_sense_frequency_control == 1)
		reg_addr += CONTROL_21_SIZE;

	/* controls 22 23 24 25 26 */
	if (f54->query.has_firmware_noise_mitigation == 1)
		reg_addr += CONTROL_22_26_SIZE;

	/* control 27 */
	if (f54->query.has_iir_filter == 1)
		reg_addr += CONTROL_27_SIZE;

	/* control 28 */
	if (f54->query.has_firmware_noise_mitigation == 1)
		reg_addr += CONTROL_28_SIZE;

	/* control 29 */
	if (f54->query.has_cmn_removal == 1)
		reg_addr += CONTROL_29_SIZE;

	/* control 30 */
	if (f54->query.has_cmn_maximum == 1)
		reg_addr += CONTROL_30_SIZE;

	/* control 31 */
	if (f54->query.has_touch_hysteresis == 1)
		reg_addr += CONTROL_31_SIZE;

	/* controls 32 33 34 35 */
	if (f54->query.has_edge_compensation == 1)
		reg_addr += CONTROL_32_35_SIZE;

	/* control 36 */
	if ((f54->query.curve_compensation_mode == 1) ||
	    (f54->query.curve_compensation_mode == 2)) {
		if (f54->query.curve_compensation_mode == 1) {
			length = max(f54->query.num_of_rx_electrodes,
				     f54->query.num_of_tx_electrodes);
		} else if (f54->query.curve_compensation_mode == 2) {
			length = f54->query.num_of_rx_electrodes;
		}
		reg_addr += CONTROL_36_SIZE * length;
	}

	/* control 37 */
	if (f54->query.curve_compensation_mode == 2)
		reg_addr += CONTROL_37_SIZE * f54->query.num_of_tx_electrodes;

	/* controls 38 39 40 */
	if (f54->query.has_per_frequency_noise_control == 1) {
		reg_addr += CONTROL_38_SIZE * num_of_sensing_freqs;
		reg_addr += CONTROL_39_SIZE * num_of_sensing_freqs;
		reg_addr += CONTROL_40_SIZE * num_of_sensing_freqs;
	}

	/* control 41 */
	if (f54->query.has_signal_clarity == 1) {
		control->reg_41 = kzalloc(sizeof(*(control->reg_41)),
					  GFP_KERNEL);
		if (!control->reg_41)
			goto exit_no_mem;
		control->reg_41->address = reg_addr;
		reg_addr += CONTROL_41_SIZE;
	}

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
		control->reg_57 = kzalloc(sizeof(*(control->reg_57)),
					  GFP_KERNEL);
		if (!control->reg_57)
			goto exit_no_mem;
		control->reg_57->address = reg_addr;
		reg_addr += CONTROL_57_SIZE;
	}

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
		reg_addr += CONTROL_75_SIZE * num_of_sensing_freqs;

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
	if ((f54->query.has_query13 == 1) && (f54->query_13.has_ctrl86 == 1)) {
		report_rate_offset = reg_addr - f54->control_base_addr;
		TS_LOG_INFO("%s, no 2 offset = %d, report_rate_offset = %d\n",
			    __func__, reg_addr, report_rate_offset);
		reg_addr += CONTROL_86_SIZE;
	}

	/* control 87 */
	if ((f54->query.has_query13 == 1) && (f54->query_13.has_ctrl87 == 1))
		reg_addr += CONTROL_87_SIZE;

	/* control 88 */
	if (f54->query.has_ctrl88 == 1) {
		control->reg_88 = kzalloc(sizeof(*(control->reg_88)),
					  GFP_KERNEL);
		if (!control->reg_88)
			goto exit_no_mem;
		control->reg_88->address = reg_addr;
		reg_addr += CONTROL_88_SIZE;
	}

	/* control 89 */
	if (f54->query_13.has_cidim ||
				f54->query_13.has_noise_mitigation_enhancement ||
				f54->query_13.has_rail_im)
	  reg_addr += CONTROL_89_SIZE;

	/* control 90 */
	if (f54->query_15.has_ctrl90)
	  reg_addr += CONTROL_90_SIZE;

	/* control 91 */
	if (f54->query_21.has_ctrl91)
	  reg_addr += CONTROL_91_SIZE;

	/* control 92 */
	if (f54->query_16.has_ctrl92)
	  reg_addr += CONTROL_92_SIZE;

	/* control 93 */
	if (f54->query_16.has_ctrl93)
	  reg_addr += CONTROL_93_SIZE;

	/* control 94 */
	if (f54->query_16.has_ctrl94_query18)
	  reg_addr += CONTROL_94_SIZE;

	/* control 95 */
	if (f54->query_16.has_ctrl95_query19)
	  reg_addr += CONTROL_95_SIZE;

	/* control 96 */
	if (f54->query_21.has_ctrl96) {
		control->reg_96 = kzalloc(sizeof(*(control->reg_96)),
					GFP_KERNEL);
		control->reg_96->address = reg_addr;
		reg_addr += CONTROL_96_SIZE;
	}

	/* control 97 */
	if (f54->query_21.has_ctrl97)
	  reg_addr += CONTROL_97_SIZE;

	/* control 98 */
	if (f54->query_21.has_ctrl98)
	  reg_addr += CONTROL_98_SIZE;

	/* control 99 */
	if (f54->query.touch_controller_family == 2)
	  reg_addr += CONTROL_99_SIZE;

	/* control 100 */
	if (f54->query_16.has_ctrl100)
	  reg_addr += CONTROL_100_SIZE;

	/* control 101 */
	if (f54->query_22.has_ctrl101)
	  reg_addr += CONTROL_101_SIZE;


	/* control 102 */
	if (f54->query_23.has_ctrl102)
	  reg_addr += CONTROL_102_SIZE;

	/* control 103 */
	if (f54->query_22.has_ctrl103_query26) {
		f54->skip_preparation = true;
		reg_addr += CONTROL_103_SIZE;
	}

	/* control 104 */
	if (f54->query_22.has_ctrl104)
	  reg_addr += CONTROL_104_SIZE;

	/* control 105 */
	if (f54->query_22.has_ctrl105)
	  reg_addr += CONTROL_105_SIZE;

	/* control 106 */
	if (f54->query_25.has_ctrl106)
	  reg_addr += CONTROL_106_SIZE;

	/* control 107 */
	if (f54->query_25.has_ctrl107)
	  reg_addr += CONTROL_107_SIZE;

	/* control 108 */
	if (f54->query_25.has_ctrl108)
	  reg_addr += CONTROL_108_SIZE;

	/* control 109 */
	if (f54->query_25.has_ctrl109)
	  reg_addr += CONTROL_109_SIZE;

	/* control 110 */
	if (f54->query_27.has_ctrl110) {
		control->reg_110 = kzalloc(sizeof(*(control->reg_110)),
					GFP_KERNEL);
		if (!control->reg_110)
		  goto exit_no_mem;
		control->reg_110->address = reg_addr;
		reg_addr += CONTROL_110_SIZE;
	}

	/* control 111 */
	if (f54->query_27.has_ctrl111)
	  reg_addr += CONTROL_111_SIZE;

	/* control 112 */
	if (f54->query_27.has_ctrl112)
	  reg_addr += CONTROL_112_SIZE;

	/* control 113 */
	if (f54->query_27.has_ctrl113)
	  reg_addr += CONTROL_113_SIZE;

	/* control 114 */
	if (f54->query_27.has_ctrl114)
	  reg_addr += CONTROL_114_SIZE;

	/* control 115 */
	if (f54->query_29.has_ctrl115)
	  reg_addr += CONTROL_115_SIZE;

	/* control 116 */
	if (f54->query_29.has_ctrl116)
	  reg_addr += CONTROL_116_SIZE;

	/* control 117 */
	if (f54->query_29.has_ctrl117)
	  reg_addr += CONTROL_117_SIZE;

	/* control 118 */
	if (f54->query_30.has_ctrl118)
	  reg_addr += CONTROL_118_SIZE;

	/* control 119 */
	if (f54->query_30.has_ctrl119)
	  reg_addr += CONTROL_119_SIZE;

	/* control 120 */
	if (f54->query_30.has_ctrl120)
	  reg_addr += CONTROL_120_SIZE;

	/* control 121 */
	if (f54->query_30.has_ctrl121)
	  reg_addr += CONTROL_121_SIZE;

	/* control 122 */
	if (f54->query_30.has_ctrl122_query31)
	  reg_addr += CONTROL_122_SIZE;

	/* control 123 */
	if (f54->query_30.has_ctrl123)
	  reg_addr += CONTROL_123_SIZE;

	/* control 124 reserved */

	/* control 125 */
	if (f54->query_32.has_ctrl125)
	  reg_addr += CONTROL_125_SIZE;

	/* control 126 */
	if (f54->query_32.has_ctrl126)
	  reg_addr += CONTROL_126_SIZE;

	/* control 127 */
	if (f54->query_32.has_ctrl127)
	  reg_addr += CONTROL_127_SIZE;

	/* controls 128 129 130 131 reserved */

	/* control 132 */
	if (f54->query_33.has_ctrl132)
	  reg_addr += CONTROL_132_SIZE;

	/* control 133 */
	if (f54->query_33.has_ctrl133)
	  reg_addr += CONTROL_133_SIZE;

	/* control 134 */
	if (f54->query_33.has_ctrl134)
	  reg_addr += CONTROL_134_SIZE;

	/* controls 135 136 reserved */

	/* control 137 */
	if (f54->query_35.has_ctrl137)
	  reg_addr += CONTROL_137_SIZE;

	/* control 138 */
	if (f54->query_35.has_ctrl138)
	  reg_addr += CONTROL_138_SIZE;

	/* control 139 */
	if (f54->query_35.has_ctrl139)
	  reg_addr += CONTROL_139_SIZE;

	/* control 140 */
	if (f54->query_35.has_ctrl140)
	  reg_addr += CONTROL_140_SIZE;

	/* control 141 reserved */

	/* control 142 */
	if (f54->query_36.has_ctrl142)
	  reg_addr += CONTROL_142_SIZE;

	/* control 143 */
	if (f54->query_36.has_ctrl143)
	  reg_addr += CONTROL_143_SIZE;

	/* control 144 */
	if (f54->query_36.has_ctrl144)
	  reg_addr += CONTROL_144_SIZE;

	/* control 145 */
	if (f54->query_36.has_ctrl145)
	  reg_addr += CONTROL_145_SIZE;

	/* control 146 */
	if (f54->query_36.has_ctrl146)
	  reg_addr += CONTROL_146_SIZE;

	/* control 147 */
	if (f54->query_38.has_ctrl147)
	  reg_addr += CONTROL_147_SIZE;

	/* control 148 */
	if (f54->query_38.has_ctrl148)
	  reg_addr += CONTROL_148_SIZE;

	/* control 149 */
	if (f54->query_38.has_ctrl149) {
		control->reg_149 = kzalloc(sizeof(*(control->reg_149)),
					GFP_KERNEL);
		if (!control->reg_149)
		  goto exit_no_mem;
		control->reg_149->address = reg_addr;
		reg_addr += CONTROL_149_SIZE;
	}

	/* controls 150 to 162 reserved */

	/* control 163 */
	if (f54->query_40.has_ctrl163_query41)
	  reg_addr += CONTROL_163_SIZE;

	/* control 164 reserved */

	/* control 165 */
	if (f54->query_40.has_ctrl165_query42)
	  reg_addr += CONTROL_165_SIZE;

	/* control 166 */
	if (f54->query_40.has_ctrl166)
	  reg_addr += CONTROL_166_SIZE;

	/* control 167 */
	if (f54->query_40.has_ctrl167)
	  reg_addr += CONTROL_167_SIZE;

	/* controls 168 to 175 reserved */

	/* control 176 */
	if (f54->query_46.has_ctrl176)
	  reg_addr += CONTROL_176_SIZE;

	/* controls 177 178 reserved */

	/* control 179 */
	if (f54->query_46.has_ctrl179)
	  reg_addr += CONTROL_179_SIZE;

	/* controls 180 to 187 reserved */

	/* control 188 */
	if (f54->query_49.has_ctrl188) {
		control->reg_188 = kzalloc(sizeof(*(control->reg_188)),
					GFP_KERNEL);
		if (!control->reg_188)
		  goto exit_no_mem;
		control->reg_188->address = reg_addr;
		reg_addr += CONTROL_188_SIZE;
	}
	return 0;

exit_no_mem:
	TS_LOG_ERR("Failed to alloc mem for control registers\n");
	return -ENOMEM;
}

static int Synaptics_test_set_queries(void)
{
	int retval = 0;
	unsigned char offset = 0;
	struct synaptics_rmi4_data *rmi4_data = f54->rmi4_data;

	retval = f54->fn_ptr->read(rmi4_data,
				   f54->query_base_addr,
				   f54->query.data, sizeof(f54->query.data));
	if (retval < 0)
		return retval;
	offset = sizeof(f54->query.data);

	/* query 12 */
	if (f54->query.has_sense_frequency_control == 0)
		offset -= 1;

	/* query 13 */
	if (f54->query.has_query13) {
		retval = f54->fn_ptr->read(rmi4_data,
					   f54->query_base_addr + offset,
					   f54->query_13.data,
					   sizeof(f54->query_13.data));
		if (retval < 0)
			return retval;
		offset += 1;
	}

	/* query 14 */
	if ((f54->query.has_query13) && (f54->query_13.has_ctrl87))
		offset += 1;

	/* query 15 */
	if (f54->query.has_query15) {
		retval = f54->fn_ptr->read(rmi4_data,
					   f54->query_base_addr + offset,
					   f54->query_15.data,
					   sizeof(f54->query_15.data));
		if (retval < 0)
			return retval;
		offset += 1;
	}

	/* query 16 */
	retval = f54->fn_ptr->read(rmi4_data,
				   f54->query_base_addr + offset,
				   f54->query_16.data,
				   sizeof(f54->query_16.data));
	if (retval < 0)
		return retval;
	offset += 1;

	/* query 17 */
	if (f54->query_16.has_query17)
		offset += 1;

	/* query 18 */
	if (f54->query_16.has_ctrl94_query18)
		offset += 1;

	/* query 19 */
	if (f54->query_16.has_ctrl95_query19)
		offset += 1;

	/* query 20 */
	if ((f54->query.has_query15) && (f54->query_15.has_query20))
		offset += 1;

	/* query 21 */
	retval = f54->fn_ptr->read(rmi4_data,
				   f54->query_base_addr + offset,
				   f54->query_21.data,
				   sizeof(f54->query_21.data));
	if (retval < 0)
		return retval;

	return 0;
}

/////////////////////////////////////////////////////////////////
static int test_f55_set_controls(void)
{
	unsigned char offset = 0;

	/* controls 0 1 2 */
	if (f55->query.has_sensor_assignment)
	  offset += 3;

	/* control 3 */
	if (f55->query.has_edge_compensation)
	  offset++;

	/* control 4 */
	if (f55->query.curve_compensation_mode == 0x1 ||
				f55->query.curve_compensation_mode == 0x2)
	  offset++;

	/* control 5 */
	if (f55->query.curve_compensation_mode == 0x2)
	  offset++;

	/* control 6 */
	if (f55->query.has_ctrl6)
	  offset++;

	/* control 7 */
	if (f55->query.has_alternate_transmitter_assignment)
	  offset++;

	/* control 8 */
	if (f55->query_3.has_ctrl8)
	  offset++;

	/* control 9 */
	if (f55->query_3.has_ctrl9)
	  offset++;

	/* control 10 */
	if (f55->query_5.has_corner_compensation)
	  offset++;

	/* control 11 */
	if (f55->query.curve_compensation_mode == 0x3)
	  offset++;

	/* control 12 */
	if (f55->query_5.has_ctrl12)
	  offset++;

	/* control 13 */
	if (f55->query_5.has_ctrl13)
	  offset++;

	/* control 14 */
	if (f55->query_5.has_ctrl14)
	  offset++;

	/* control 15 */
	if (f55->query_5.has_basis_function)
	  offset++;

	/* control 16 */
	if (f55->query_17.has_ctrl16)
	  offset++;

	/* control 17 */
	if (f55->query_17.has_ctrl17)
	  offset++;

	/* controls 18 19 */
	if (f55->query_17.has_ctrl18_ctrl19)
	  offset += 2;

	/* control 20 */
	if (f55->query_17.has_ctrl20)
	  offset++;

	/* control 21 */
	if (f55->query_17.has_ctrl21)
	  offset++;

	/* control 22 */
	if (f55->query_17.has_ctrl22)
	  offset++;

	/* control 23 */
	if (f55->query_18.has_ctrl23)
	  offset++;

	/* control 24 */
	if (f55->query_18.has_ctrl24)
	  offset++;

	/* control 25 */
	if (f55->query_18.has_ctrl25)
	  offset++;

	/* control 26 */
	if (f55->query_18.has_ctrl26)
	  offset++;

	/* control 27 */
	if (f55->query_18.has_ctrl27_query20)
	  offset++;

	/* control 28 */
	if (f55->query_18.has_ctrl28_query21)
	  offset++;

	/* control 29 */
	if (f55->query_22.has_ctrl29)
	  offset++;

	/* control 30 */
	if (f55->query_22.has_ctrl30)
	  offset++;

	/* control 31 */
	if (f55->query_22.has_ctrl31)
	  offset++;

	/* control 32 */
	if (f55->query_22.has_ctrl32)
	  offset++;

	/* controls 33 34 35 36 reserved */

	/* control 37 */
	if (f55->query_28.has_ctrl37)
	  offset++;

	/* control 38 */
	if (f55->query_30.has_ctrl38)
	  offset++;

	/* control 39 */
	if (f55->query_30.has_ctrl39)
	  offset++;

	/* control 40 */
	if (f55->query_30.has_ctrl40)
	  offset++;

	/* control 41 */
	if (f55->query_30.has_ctrl41)
	  offset++;

	/* control 42 */
	if (f55->query_30.has_ctrl42)
	  offset++;

	/* controls 43 44 */
	if (f55->query_30.has_ctrl43_ctrl44) {
		f55->afe_mux_offset = offset;
		offset += 2;
	}

	/* controls 45 46 */
	if (f55->query_33.has_ctrl45_ctrl46) {
		f55->has_force = true;
		f55->force_tx_offset = offset;
		f55->force_rx_offset = offset + 1;
		offset += 2;
	}

	return 0;
}

static int test_f55_set_queries(void)
{
	int retval = 0;
	unsigned char offset = 0;
	struct synaptics_rmi4_data *rmi4_data = f54->rmi4_data;

	retval = f54->fn_ptr->read(rmi4_data,
				f55->query_base_addr,
				f55->query.data,
				sizeof(f55->query.data));
	if (retval < 0)
	  return retval;

	offset = sizeof(f55->query.data);

	/* query 3 */
	if (f55->query.has_single_layer_multi_touch) {
		retval = f54->fn_ptr->read(rmi4_data,
					f55->query_base_addr + offset,
					f55->query_3.data,
					sizeof(f55->query_3.data));
		if (retval < 0)
		  return retval;
		offset += 1;
	}

	/* query 4 */
	if (f55->query_3.has_ctrl9)
	  offset += 1;

	/* query 5 */
	if (f55->query.has_query5) {
		retval = f54->fn_ptr->read(rmi4_data,
					f55->query_base_addr + offset,
					f55->query_5.data,
					sizeof(f55->query_5.data));
		if (retval < 0)
		  return retval;
		offset += 1;
	}

	/* queries 6 7 */
	if (f55->query.curve_compensation_mode == 0x3)
	  offset += 2;

	/* query 8 */
	if (f55->query_3.has_ctrl8)
	  offset += 1;

	/* query 9 */
	if (f55->query_3.has_query9)
	  offset += 1;

	/* queries 10 11 12 13 14 15 16 */
	if (f55->query_5.has_basis_function)
	  offset += 7;

	/* query 17 */
	if (f55->query_5.has_query17) {
		retval = f54->fn_ptr->read(rmi4_data,
					f55->query_base_addr + offset,
					f55->query_17.data,
					sizeof(f55->query_17.data));
		if (retval < 0)
		  return retval;
		offset += 1;
	}

	/* query 18 */
	if (f55->query_17.has_query18) {
		retval = f54->fn_ptr->read(rmi4_data,
					f55->query_base_addr + offset,
					f55->query_18.data,
					sizeof(f55->query_18.data));
		if (retval < 0)
		  return retval;
		offset += 1;
	}

	/* query 19 */
	if (f55->query_18.has_query19)
	  offset += 1;

	/* query 20 */
	if (f55->query_18.has_ctrl27_query20)
	  offset += 1;

	/* query 21 */
	if (f55->query_18.has_ctrl28_query21)
	  offset += 1;

	/* query 22 */
	if (f55->query_18.has_query22) {
		retval = f54->fn_ptr->read(rmi4_data,
					f55->query_base_addr + offset,
					f55->query_22.data,
					sizeof(f55->query_22.data));
		if (retval < 0)
		  return retval;
		offset += 1;
	}

	/* query 23 */
	if (f55->query_22.has_query23) {
		retval = f54->fn_ptr->read(rmi4_data,
					f55->query_base_addr + offset,
					f55->query_23.data,
					sizeof(f55->query_23.data));
		if (retval < 0)
		  return retval;
		offset += 1;

		f55->amp_sensor = f55->query_23.amp_sensor_enabled;
		f55->size_of_column2mux = f55->query_23.size_of_column2mux;
	}

	/* queries 24 25 26 27 reserved */

	/* query 28 */
	if (f55->query_22.has_query28) {
		retval = f54->fn_ptr->read(rmi4_data,
					f55->query_base_addr + offset,
					f55->query_28.data,
					sizeof(f55->query_28.data));
		if (retval < 0)
		  return retval;
		offset += 1;
	}

	/* query 29 */
	if (f55->query_28.has_query29)
	  offset += 1;

	/* query 30 */
	if (f55->query_28.has_query30) {
		retval = f54->fn_ptr->read(rmi4_data,
					f55->query_base_addr + offset,
					f55->query_30.data,
					sizeof(f55->query_30.data));
		if (retval < 0)
		  return retval;
		offset += 1;
	}

	/* queries 31 32 */
	if (f55->query_30.has_query31_query32)
	  offset += 2;

	/* query 33 */
	if (f55->query_30.has_query33) {
		retval = f54->fn_ptr->read(rmi4_data,
					f55->query_base_addr + offset,
					f55->query_33.data,
					sizeof(f55->query_33.data));
		if (retval < 0)
		  return retval;
		offset += 1;

		f55->extended_amp = f55->query_33.has_extended_amp_pad;
	}

	return 0;
}

static int test_f55_init(struct synaptics_rmi4_data *rmi4_data)
{
	int retval = -1;
	unsigned char ii = 0;
	unsigned char rx_electrodes = 0;
	unsigned char tx_electrodes = 0;
	struct f55_control_43 ctrl_43;

	retval = test_f55_set_queries();
	if (retval < 0) {
		TS_LOG_ERR("%s: Failed to read F55 query registers\n",
					__func__);
		return retval;
	}

	if (!f55->query.has_sensor_assignment) {
		TS_LOG_ERR("%s: has_sensor_assignment false\n",
					__func__);
		return -1;
	}

	retval = test_f55_set_controls();
	if (retval < 0) {
		TS_LOG_ERR("%s: Failed to set up F55 control registers\n",
					__func__);
		return retval;
	}

	tx_electrodes = f55->query.num_of_tx_electrodes;
	rx_electrodes = f55->query.num_of_rx_electrodes;

	f55->tx_assignment = kzalloc(tx_electrodes, GFP_KERNEL);
	f55->rx_assignment = kzalloc(rx_electrodes, GFP_KERNEL);
	f55->rx_physical = kzalloc(rx_electrodes, GFP_KERNEL);

	retval = f54->fn_ptr->read(rmi4_data,
				f55->control_base_addr + SENSOR_TX_MAPPING_OFFSET,
				f55->tx_assignment,
				tx_electrodes);
	if (retval < 0) {
		TS_LOG_ERR("%s: Failed to read F55 tx assignment\n",
					__func__);
		return retval;
	}

	retval = f54->fn_ptr->read(rmi4_data,
				f55->control_base_addr + SENSOR_RX_MAPPING_OFFSET,
				f55->rx_assignment,
				rx_electrodes);
	if (retval < 0) {
		TS_LOG_ERR("%s: Failed to read F55 rx assignment\n",
					__func__);
		return retval;
	}

	f54->tx_assigned = 0;
	for (ii = 0; ii < tx_electrodes; ii++) {
		if (f55->tx_assignment[ii] != 0xff)
		  f54->tx_assigned++;
	}

	f54->rx_assigned = 0;
	for (ii = 0; ii < rx_electrodes; ii++) {
		if (f55->rx_assignment[ii] != 0xff) {
			f54->rx_assigned++;
			f55->rx_physical[ii] = f55->rx_assignment[ii];
		}
	}

	if (f55->amp_sensor) {
		f54->tx_assigned = f55->size_of_column2mux;
		f54->rx_assigned /= 2;
	}

	if (f55->extended_amp) {
		retval = f54->fn_ptr->read(rmi4_data,
					f55->control_base_addr + f55->afe_mux_offset,
					ctrl_43.data,
					sizeof(ctrl_43.data));
		if (retval < 0) {
			TS_LOG_ERR("%s: Failed to read F55 AFE mux sizes\n",
						__func__);
			return retval;
		}

		f54->tx_assigned = ctrl_43.afe_l_mux_size +
			ctrl_43.afe_r_mux_size;
	}

	/* force mapping */
	if (f55->has_force) {
		f55->force_tx_assignment = kzalloc(tx_electrodes, GFP_KERNEL);
		f55->force_rx_assignment = kzalloc(rx_electrodes, GFP_KERNEL);

		retval = f54->fn_ptr->read(rmi4_data,
					f55->control_base_addr + f55->force_tx_offset,
					f55->force_tx_assignment,
					tx_electrodes);
		if (retval < 0) {
			TS_LOG_ERR("%s: Failed to read F55 force tx assignment\n",
						__func__);
			return retval;
		}

		retval = f54->fn_ptr->read(rmi4_data,
					f55->control_base_addr + f55->force_rx_offset,
					f55->force_rx_assignment,
					rx_electrodes);
		if (retval < 0) {
			TS_LOG_ERR("%s: Failed to read F55 force rx assignment\n",
						__func__);
			return retval;
		}

		for (ii = 0; ii < tx_electrodes; ii++) {
			if (f55->force_tx_assignment[ii] != 0xff)
			  f54->tx_assigned++;
		}

		for (ii = 0; ii < rx_electrodes; ii++) {
			if (f55->force_rx_assignment[ii] != 0xff)
			  f54->rx_assigned++;
		}
	}

	return 0;
}

/////////////////////////////////////////////////////////////////
static int match_module_name(const char *module_name)
{
	TS_LOG_INFO("%s: module_name = %s,product_name=%s\n", __func__,
		    module_name, rmi4_data->synaptics_chip_data->ts_platform_data->product_name);
	if (strcmp(rmi4_data->synaptics_chip_data->ts_platform_data->product_name, "chm") == 0) {
		if (strcmp(module_name, "oflim") == 0) {
			RawCapUpperLimit = RawCapUpperLimit_oflim_chm;
			RawCapLowerLimit = RawCapLowerLimit_oflim_chm;
			RxDeltaCapUpperLimit = RxDeltaCapUpperLimit_oflim_chm;
			RxDeltaCapLowerLimit = RxDeltaCapLowerLimit_oflim_chm;
			TxDeltaCapUpperLimit = TxDeltaCapUpperLimit_oflim_chm;
			TxDeltaCapLowerLimit = TxDeltaCapLowerLimit_oflim_chm;
			return NO_ERR;
		} else if (strcmp(module_name, "lensone") == 0) {
			RawCapUpperLimit = RawCapUpperLimit_lensone_chm;
			RawCapLowerLimit = RawCapLowerLimit_lensone_chm;
			RxDeltaCapUpperLimit = RxDeltaCapUpperLimit_lensone_chm;
			RxDeltaCapLowerLimit = RxDeltaCapLowerLimit_lensone_chm;
			TxDeltaCapUpperLimit = TxDeltaCapUpperLimit_lensone_chm;
			TxDeltaCapLowerLimit = TxDeltaCapLowerLimit_lensone_chm;
			return NO_ERR;
		} else if (strcmp(module_name, "truly") == 0) {
			RawCapUpperLimit = RawCapUpperLimit_truly_chm;
			RawCapLowerLimit = RawCapLowerLimit_truly_chm;
			RxDeltaCapUpperLimit = RxDeltaCapUpperLimit_truly_chm;
			RxDeltaCapLowerLimit = RxDeltaCapLowerLimit_truly_chm;
			TxDeltaCapUpperLimit = TxDeltaCapUpperLimit_truly_chm;
			TxDeltaCapLowerLimit = TxDeltaCapLowerLimit_truly_chm;
			return NO_ERR;
		} else {
			TS_LOG_ERR("%s: Failed to match module_name \n",
				   __func__);
			return -EINVAL;
		}
	}else {
		TS_LOG_ERR("%s: Failed to match module_name \n",
				   __func__);
		return -EINVAL;
	}
}

int synap_rmi4_f54_init(struct synaptics_rmi4_data *rmi4_data,
			    const char *module_name)
{
	int retval = -EINVAL;
	bool hasF54 = false;
	bool hasF55 = false;
	bool hasF34 = false;
	unsigned short ii = 0;
	unsigned char page = 0;
	unsigned char intr_count = 0;
	struct synaptics_rmi4_fn_desc rmi_fd;

	if (synaptics_raw_data_limit_flag) {
		retval = match_module_name(module_name);
		if (retval < 0) {
			retval = -ENOMEM;
			return retval;
		}
	}
	if (synaptics_f54_malloc() != NO_ERR)
		goto exit_free_mem;

	f54->rmi4_data = rmi4_data;
	f54->fn_ptr->read = rmi4_data->i2c_read;
	f54->fn_ptr->write = rmi4_data->i2c_write;

	for (page = 0; page < PAGES_TO_SERVICE; page++) {
		for (ii = PDT_START; ii > PDT_END; ii -= PDT_ENTRY_SIZE) {
			ii |= (page << 8);

			retval =
			    f54->fn_ptr->read(rmi4_data, ii,
					      (unsigned char *)&rmi_fd,
					      sizeof(rmi_fd));
			if (retval < 0) {
				TS_LOG_ERR
				    ("i2c read error, page = %d, ii = %d\n",
				     page, ii);
				goto exit_free_mem;
			}

			if (!rmi_fd.fn_number) {
				TS_LOG_INFO("!rmi_fd.fn_number,page=%d,ii=%d\n",
					    page, ii);
				retval = -EINVAL;
				break;
			}

			if (rmi_fd.fn_number == SYNAPTICS_RMI4_F54) {
				hasF54 = true;
				f54->query_base_addr =
				    rmi_fd.query_base_addr | (page << 8);
				f54->control_base_addr =
				    rmi_fd.ctrl_base_addr | (page << 8);
				f54->data_base_addr =
				    rmi_fd.data_base_addr | (page << 8);
				f54->command_base_addr =
				    rmi_fd.cmd_base_addr | (page << 8);
				TS_LOG_DEBUG
				    ("f54->control_base_addr = 0x%04x, f54->data_base_addr = 0x%04x, f54->query_base_addr = 0x%04x\n",
				     f54->control_base_addr,
				     f54->data_base_addr, f54->query_base_addr);
			} else if (rmi_fd.fn_number == SYNAPTICS_RMI4_F55) {
				hasF55 = true;
				f54->fn55->query_base_addr =
				    rmi_fd.query_base_addr | (page << 8);
				f54->fn55->control_base_addr =
				    rmi_fd.ctrl_base_addr | (page << 8);
				f55->query_base_addr =
				    rmi_fd.query_base_addr | (page << 8);
				f55->control_base_addr =
				    rmi_fd.ctrl_base_addr | (page << 8);
				f55->data_base_addr =
				    rmi_fd.data_base_addr | (page << 8);
				f55->command_base_addr =
				    rmi_fd.cmd_base_addr | (page << 8);
			} else if (rmi_fd.fn_number == SYNAPTICS_RMI4_F34) {
				hasF34 = true;
				f54->f34_fd.query_base_addr =
				    rmi_fd.query_base_addr;
				f54->f34_fd.ctrl_base_addr =
				    rmi_fd.ctrl_base_addr;
				f54->f34_fd.data_base_addr =
				    rmi_fd.data_base_addr;
			}

			if (hasF54 && hasF55 && hasF34) {
				TS_LOG_INFO("%s: goto found\n", __func__);
				goto found;
			}

			if (!hasF54)
				intr_count +=
				    (rmi_fd.intr_src_count & MASK_3BIT);
		}
	}

	TS_LOG_INFO
	    ("f54->control_base_addr = 0x%04x, f54->data_base_addr = 0x%04x\n",
	     f54->control_base_addr, f54->data_base_addr);

	if (!hasF54 || !hasF34) {
		TS_LOG_ERR
		    ("Funtion  is not available, hasF54=%d, hasF34 = %d\n",
		     hasF54, hasF34);
		retval = -EINVAL;
		goto exit_free_mem;
	}

found:
	retval =
	    f54->fn_ptr->read(rmi4_data, f54->query_base_addr, f54->query.data,
			      sizeof(f54->query.data));
	if (retval < 0) {
		TS_LOG_ERR("Failed to read query registers\n");
		goto exit_free_mem;
	}

	retval = Synaptics_test_set_queries();
	if (retval < 0) {
		TS_LOG_ERR("Failed to set up f54 queries registers\n");
		goto exit_free_mem;
	}

	retval = Synaptics_test_set_controls();
	if (retval < 0) {
		TS_LOG_ERR("Failed to set up f54 control registers\n");
		goto exit_free_control;
	}

	if (hasF55) {
		retval = test_f55_init(rmi4_data);
		if (retval < 0) {
			TS_LOG_ERR("Failed to set up f55 init\n");
			goto exit_free_control;
		}
	}

	retval = synaptics_read_f34();
	if (retval) {
		TS_LOG_ERR("Read F34 failed, retval = %d\n", retval);
		goto exit_free_mem;
	}

	f54->mmi_buf = (char *)kzalloc(mmi_buf_size, GFP_KERNEL);
	if (!f54->mmi_buf) {
		TS_LOG_ERR("Failed to alloc mmi_buf\n");
		retval = -ENOMEM;
		goto exit_free_mem;
	}

	f54->rawdatabuf =
	    (int *)kzalloc(rawdata_size * sizeof(int), GFP_KERNEL);
	if (!f54->rawdatabuf) {
		TS_LOG_ERR(" Failed to alloc rawdatabuf\n");
		retval = -ENOMEM;
		goto exit_free_mem;
	}
	f54->hybridbuf =
			(u32 *)kzalloc((HYBRID_BUF_SIZE) * sizeof(int), GFP_KERNEL);
	if (!f54->hybridbuf) {
		TS_LOG_ERR(" Failed to alloc hybridbuf\n");
		retval = -ENOMEM;
		goto exit_free_mem;
	}

	return NO_ERR;

exit_free_control:
	Synaptics_test_free_control_mem();

exit_free_mem:
	synaptics_f54_free();
	return retval;
}

unsigned short synap_f54_get_calibrate_addr(struct synaptics_rmi4_data
						*rmi4_data,
						const char *module_name)
{
	int rc = NO_ERR;
	u8 value = 0;
	unsigned short addr = 0;

	TS_LOG_INFO("%s called\n", __func__);

	rc = synap_rmi4_f54_init(rmi4_data, module_name);
	if (rc < 0) {
		TS_LOG_ERR("Failed to init f54\n");
		goto exit;
	}

	rc = f54->fn_ptr->read(rmi4_data, f54->query_base_addr + 0x0B, &value,
			       sizeof(value));
	if (rc) {
		TS_LOG_ERR("Read F54 ESD check failed, retval = %d\n", rc);
		goto exit;
	}

	addr = f54->control_base_addr + 0x1d;

	if (value & 0x10) {
		addr += 2;
	}

	TS_LOG_INFO("%s, esd value is 0x%02x, addr is 0x%02x\n", __func__,
		    value, addr);

exit:
	synaptics_f54_free();
	return addr;
}
