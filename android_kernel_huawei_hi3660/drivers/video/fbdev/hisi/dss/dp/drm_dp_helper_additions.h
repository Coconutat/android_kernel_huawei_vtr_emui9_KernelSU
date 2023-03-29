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

#ifndef __DRM_DP_HELPER_ADDITIONS_H__
#define __DRM_DP_HELPER_ADDITIONS_H__

/*
 * The following aren't yet defined in kernel headers
 */

#define DP_LINK_BW_8_1				0x1e
#define DP_TRAINING_PATTERN_4			7
#define DP_TPS4_SUPPORTED			BIT(7)

#define DP_TEST_LINK_AUDIO_PATTERN		BIT(5)
#define DP_TEST_H_TOTAL_MSB                     0x222
#define DP_TEST_H_TOTAL_LSB			0x223
#define DP_TEST_V_TOTAL_MSB                     0x224
#define DP_TEST_V_TOTAL_LSB			0x225
#define DP_TEST_H_START_MSB			0x226
#define DP_TEST_H_START_LSB			0x227
#define DP_TEST_V_START_MSB			0x228
#define DP_TEST_V_START_LSB			0x229
#define DP_TEST_H_SYNC_WIDTH_MSB		0x22A
#define DP_TEST_H_SYNC_WIDTH_LSB		0x22B
#define DP_TEST_V_SYNC_WIDTH_MSB		0x22C
#define DP_TEST_V_SYNC_WIDTH_LSB		0x22D
#define DP_TEST_H_WIDTH_MSB			0x22E
#define DP_TEST_H_WIDTH_LSB			0x22F
#define DP_TEST_V_WIDTH_MSB			0x230
#define DP_TEST_V_WIDTH_LSB			0x231
#define DP_TEST_PHY_PATTERN			0x248
#define DP_TEST_PATTERN_NONE			0x0
#define DP_TEST_PATTERN_COLOR_RAMPS		0x1
#define DP_TEST_PATTERN_BW_VERITCAL_LINES	0x2
#define DP_TEST_PATTERN_COLOR_SQUARE		0x3

#define DP_TEST_PHY_PATTERN_SEL_MASK		GENMASK(2, 0)
#define DP_TEST_PHY_PATTERN_NONE		0x0
#define DP_TEST_PHY_PATTERN_D10			0x1
#define DP_TEST_PHY_PATTERN_SEMC		0x2
#define DP_TEST_PHY_PATTERN_PRBS7		0x3
#define DP_TEST_PHY_PATTERN_CUSTOM		0x4
#define DP_TEST_PHY_PATTERN_CP2520		0x5

#define DP_TEST_MISC				0x232
#define DP_TEST_COLOR_FORMAT_MASK		GENMASK(2, 1)
#define DP_TEST_DYNAMIC_RANGE_SHIFT             3
#define DP_TEST_DYNAMIC_RANGE_MASK		BIT(3)
#define DP_TEST_YCBCR_COEFF_SHIFT		4
#define DP_TEST_YCBCR_COEFF_MASK		BIT(4)
#define DP_TEST_BIT_DEPTH_SHIFT			5
#define DP_TEST_BIT_DEPTH_MASK                  GENMASK(7, 5)

#define DP_TEST_BIT_DEPTH_6			0x0
#define DP_TEST_BIT_DEPTH_8			0x1
#define DP_TEST_BIT_DEPTH_10			0x2
#define DP_TEST_BIT_DEPTH_12			0x3
#define DP_TEST_BIT_DEPTH_16			0x4
#define DP_TEST_DYNAMIC_RANGE_VESA		0x0
#define DP_TEST_DYNAMIC_RANGE_CEA               0x1
#define DP_TEST_COLOR_FORMAT_RGB	        0x0
#define DP_TEST_COLOR_FORMAT_YCBCR422           0x2
#define DP_TEST_COLOR_FORMAT_YCBCR444		0x4
#define DP_TEST_YCBCR_COEFF_ITU601		0x0
#define DP_TEST_YCBCR_COEFF_ITU709		0x1

#define	DP_TEST_AUDIO_MODE			0x271
#define DP_TEST_AUDIO_SAMPLING_RATE_MASK	GENMASK(3, 0)
#define DP_TEST_AUDIO_CH_COUNT_SHIFT		4
#define DP_TEST_AUDIO_CH_COUNT_MASK		GENMASK(7, 4)

#define DP_TEST_AUDIO_SAMPLING_RATE_32		0x0
#define DP_TEST_AUDIO_SAMPLING_RATE_44_1	0x1
#define DP_TEST_AUDIO_SAMPLING_RATE_48		0x2
#define DP_TEST_AUDIO_SAMPLING_RATE_88_2	0x3
#define DP_TEST_AUDIO_SAMPLING_RATE_96		0x4
#define DP_TEST_AUDIO_SAMPLING_RATE_176_4	0x5
#define DP_TEST_AUDIO_SAMPLING_RATE_192		0x6

#define DP_TEST_AUDIO_CHANNEL1			0x0
#define DP_TEST_AUDIO_CHANNEL2			0x1
#define DP_TEST_AUDIO_CHANNEL3			0x2
#define DP_TEST_AUDIO_CHANNEL4			0x3
#define DP_TEST_AUDIO_CHANNEL5			0x4
#define DP_TEST_AUDIO_CHANNEL6			0x5
#define DP_TEST_AUDIO_CHANNEL7			0x6
#define DP_TEST_AUDIO_CHANNEL8			0x7

static inline bool
drm_dp_tps4_supported(const uint8_t dpcd[DP_RECEIVER_CAP_SIZE])
{
	return dpcd[DP_DPCD_REV] >= 0x14 &&
		dpcd[DP_MAX_DOWNSPREAD] & DP_TPS4_SUPPORTED;
}

static inline bool
drm_dp_tps3_supported(const uint8_t dpcd[DP_RECEIVER_CAP_SIZE])
{
         return dpcd[DP_DPCD_REV] >= 0x12 &&
                 dpcd[DP_MAX_LANE_COUNT] & DP_TPS3_SUPPORTED;
}

#endif
