/*
 * Synaptics TCM touchscreen driver
 *
 * Copyright (C) 2017-2018 Synaptics Incorporated. All rights reserved.
 *
 * Copyright (C) 2017-2018 Scott Lin <scott.lin@tw.synaptics.com>
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
 * INFORMATION CONTAINED IN THIS DOCUMENT IS PROVIDED "AS-IS," AND SYNAPTICS
 * EXPRESSLY DISCLAIMS ALL EXPRESS AND IMPLIED WARRANTIES, INCLUDING ANY
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE,
 * AND ANY WARRANTIES OF NON-INFRINGEMENT OF ANY INTELLECTUAL PROPERTY RIGHTS.
 * IN NO EVENT SHALL SYNAPTICS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, PUNITIVE, OR CONSEQUENTIAL DAMAGES ARISING OUT OF OR IN CONNECTION
 * WITH THE USE OF THE INFORMATION CONTAINED IN THIS DOCUMENT, HOWEVER CAUSED
 * AND BASED ON ANY THEORY OF LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * NEGLIGENCE OR OTHER TORTIOUS ACTION, AND EVEN IF SYNAPTICS WAS ADVISED OF
 * THE POSSIBILITY OF SUCH DAMAGE. IF A TRIBUNAL OF COMPETENT JURISDICTION DOES
 * NOT PERMIT THE DISCLAIMER OF DIRECT DAMAGES OR ANY OTHER DAMAGES, SYNAPTICS'
 * TOTAL CUMULATIVE LIABILITY TO ANY PARTY SHALL NOT EXCEED ONE HUNDRED U.S.
 * DOLLARS.
 */

#ifndef _SYNAPTICS_TCM_TESTING_H_
#define _SYNAPTICS_TCM_TESTING_H_

#define CSV_RAW_DATA_MIN_ARRAY		"threshold,raw_data_min_array"
#define CSV_RAW_DATA_MAX_ARRAY		"threshold,raw_data_max_array"
#define CSV_OPEN_SHORT_MIN_ARRAY	"threshold,open_short_min_array"
#define CSV_OPEN_SHORT_MAX_ARRAY	"threshold,open_short_max_array"
#define CSV_LCD_NOISE_ARRAY			"threshold,lcd_noise_max_array"
#define CSV_RAW_DATA_TRX_DELTA_MAX_ARRAY	"threshold,raw_data_Trx_delta_max_array"
#define CSV_RAW_DATA_COL_MIN_ARRAY		"threshold,raw_data_col_min_array"
#define CSV_RAW_DATA_COL_MAX_ARRAY		"threshold,raw_data_col_max_array"
#define CSV_RAW_DATA_ROW_MIN_ARRAY		"threshold,raw_data_row_min_array"
#define CSV_RAW_DATA_ROW_MAX_ARRAY		"threshold,raw_data_row_max_array"

#define TX_NUM_MAX			40
#define RX_NUM_MAX			40

#define CAP_TEST_FAIL_CHAR 'F'
#define CAP_TEST_PASS_CHAR 'P'

struct syna_tcm_test_threshold {
	int32_t raw_data_min_limits[TX_NUM_MAX*RX_NUM_MAX];
	int32_t raw_data_max_limits[TX_NUM_MAX*RX_NUM_MAX];
	int32_t open_short_min_limits[TX_NUM_MAX*RX_NUM_MAX];
	int32_t open_short_max_limits[TX_NUM_MAX*RX_NUM_MAX];
	int32_t lcd_noise_max_limits[TX_NUM_MAX*RX_NUM_MAX];
	int32_t raw_data_Trx_delta_max_limits[TX_NUM_MAX*RX_NUM_MAX*2];
	int32_t raw_data_col_min_limits[TX_NUM_MAX];
	int32_t raw_data_col_max_limits[TX_NUM_MAX];
	int32_t raw_data_row_min_limits[TX_NUM_MAX];
	int32_t raw_data_row_max_limits[TX_NUM_MAX];
};

struct syna_tcm_test_params {
	bool cap_thr_is_parsed;
	struct syna_tcm_test_threshold threshold;
	u32 raw_data_test;
	u32 noise_test;
	u32 open_short_test;
};

#endif
