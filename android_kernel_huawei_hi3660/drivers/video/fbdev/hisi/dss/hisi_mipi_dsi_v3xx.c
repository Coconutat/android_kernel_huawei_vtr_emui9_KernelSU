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

#include "hisi_mipi_dsi.h"

#define ROUND1(x,y)	((x) / (y) + ((x) % (y) > 0 ? 1 : 0))
#define DSS_REDUCE(x)	((x) > 0 ? ((x) - 1) : (x))

struct dsi_phy_seq_info dphy_seq_info[] = {
	{47, 94, 0, 7},
	{94, 188, 0, 6},
	{188, 375, 0, 5},
	{375, 750, 0, 4},
	{750, 1500, 0, 0}
};
/*lint -e647 */
static void get_dsi_phy_ctrl(struct hisi_fb_data_type *hisifd,
	struct mipi_dsi_phy_ctrl *phy_ctrl)
{
	struct hisi_panel_info *pinfo = NULL;
	uint32_t dsi_bit_clk = 0;

	uint32_t ui = 0;
	uint32_t m_pll = 0;
	uint32_t n_pll = 0;
	uint32_t m_n_fract =0;
	uint32_t m_n_int =0;
	uint64_t lane_clock = 0;
	uint64_t vco_div = 1;
	uint32_t m_pll_remainder[3] = {0};
	uint32_t i = 0;
	uint32_t temp_min;

	uint32_t accuracy = 0;
	uint32_t unit_tx_byte_clk_hs = 0;
	uint32_t clk_post = 0;
	uint32_t clk_pre =0;
	uint32_t clk_t_hs_exit = 0;
	uint32_t clk_pre_delay = 0;
	uint32_t clk_t_hs_prepare = 0;
	uint32_t clk_t_lpx = 0;
	uint32_t clk_t_hs_zero = 0;
	uint32_t clk_t_hs_trial = 0;
	uint32_t data_post_delay = 0;
	uint32_t data_t_hs_prepare = 0;
	uint32_t data_t_hs_zero = 0;
	uint32_t data_t_hs_trial = 0;
	uint32_t data_t_lpx = 0;
	uint32_t clk_pre_delay_reality = 0;
	uint32_t clk_t_hs_zero_reality = 0;
	uint32_t clk_post_delay_reality = 0;
	uint32_t data_t_hs_zero_reality = 0;
	uint32_t data_post_delay_reality = 0;
	uint32_t data_pre_delay_reality = 0;

	if ((NULL == phy_ctrl) || (NULL == hisifd)) {
		return;
	}
	pinfo = &(hisifd->panel_info);

	dsi_bit_clk = pinfo->mipi.dsi_bit_clk_upt;
	lane_clock = 2 * dsi_bit_clk;
	HISI_FB_DEBUG("Expected : lane_clock = %llu M\n", lane_clock);

	/************************  PLL parameters config  *********************/
	//chip spec :
	//If the output data rate is below 320 Mbps, RG_BNAD_SEL should be set to 1.
	//At this mode a post divider of 1/4 will be applied to VCO.
	if ((320 <= lane_clock) && (lane_clock <= 2500)) {
		phy_ctrl->rg_band_sel = 0;	//0x1E[2]
		vco_div = 1;
	} else if ((80 <= lane_clock) && (lane_clock <320)) {
		phy_ctrl->rg_band_sel = 1;
		vco_div = 4;
	} else {
		HISI_FB_ERR("80M <= lane_clock< = 2500M, not support lane_clock = %llu M\n", lane_clock);
	}
	if(pinfo->mipi.phy_m_n_count_update) {
		for (i = 0;i < 3;i++) {
			n_pll = i + 1;
			m_pll_remainder[i] = ((n_pll * lane_clock * 1000000UL * 1000UL / DEFAULT_MIPI_CLK_RATE) % 1000) * 10 / 1000;
		}
		temp_min = m_pll_remainder[0];
		n_pll = 1;
		for (i =1;i < 3;i++) {
			if (temp_min > m_pll_remainder[i]) {
				temp_min = m_pll_remainder[i];
				n_pll = i + 1;
			}
		}
		m_pll = n_pll * lane_clock * vco_div * 1000000UL / DEFAULT_MIPI_CLK_RATE;
		HISI_FB_DEBUG("m_pll = %d n_pll  =  %d \n", m_pll,n_pll);

	} else {

		m_n_int = lane_clock * vco_div * 1000000UL / DEFAULT_MIPI_CLK_RATE;
		m_n_fract = ((lane_clock * vco_div * 1000000UL * 1000UL / DEFAULT_MIPI_CLK_RATE) % 1000) * 10 / 1000;

		if (m_n_int % 2 == 0) {
			if (m_n_fract * 6 >= 50) {
				n_pll = 2;
				m_pll = (m_n_int + 1) * n_pll;
			} else if (m_n_fract * 6 >= 30) {
				n_pll = 3;
				m_pll = m_n_int * n_pll + 2;
			} else {
				n_pll = 1;
				m_pll = m_n_int * n_pll;
			}
		} else {
			if (m_n_fract * 6 >= 50) {
				n_pll = 1;
				m_pll = (m_n_int + 1) * n_pll;
			} else if (m_n_fract * 6 >= 30) {
				n_pll = 1;
				m_pll = (m_n_int + 1) * n_pll;
			} else if (m_n_fract * 6 >= 10) {
				n_pll = 3;
				m_pll = m_n_int * n_pll + 1;
			} else {
				n_pll = 2;
				m_pll = m_n_int * n_pll;
			}
		}
	}
	//if set rg_pll_enswc=1, rg_pll_fbd_s can't be 0
	if (m_pll <= 8) {
		phy_ctrl->rg_pll_fbd_s = 1;
		phy_ctrl->rg_pll_enswc = 0;

		if (m_pll % 2 == 0) {
			phy_ctrl->rg_pll_fbd_p = m_pll / 2;
		} else {
			if (n_pll == 1) {
				n_pll *= 2;
				phy_ctrl->rg_pll_fbd_p = (m_pll  * 2) / 2;
			} else {
				HISI_FB_ERR("phy m_pll not support!m_pll = %d\n", m_pll);
				return;
			}
		}
	} else if (m_pll <= 300) {
		if (m_pll % 2 == 0)
			phy_ctrl->rg_pll_enswc = 0;
		else
			phy_ctrl->rg_pll_enswc = 1;

		phy_ctrl->rg_pll_fbd_s = 1;
		phy_ctrl->rg_pll_fbd_p = m_pll / 2;
	} else if (m_pll <= 315) {
		phy_ctrl->rg_pll_fbd_p = 150;
		phy_ctrl->rg_pll_fbd_s = m_pll - 2 * phy_ctrl->rg_pll_fbd_p;
		phy_ctrl->rg_pll_enswc = 1;
	} else {
		HISI_FB_ERR("phy m_pll not support!m_pll = %d\n", m_pll);
		return;
	}

	phy_ctrl->rg_pll_pre_p = n_pll;

	lane_clock = m_pll * (DEFAULT_MIPI_CLK_RATE / n_pll) / vco_div;
	HISI_FB_DEBUG("Config : lane_clock = %llu\n", lane_clock);

	//FIXME :
	phy_ctrl->rg_pll_cp = 1;		//0x16[7:5]
	phy_ctrl->rg_pll_cp_p = 3;		//0x1E[7:5]

	//test_code_0x14 other parameters config
	phy_ctrl->rg_pll_enbwt = 0;	//0x14[2]
	phy_ctrl->rg_pll_chp = 0;		//0x14[1:0]

	//test_code_0x16 other parameters config,  0x16[3:2] reserved
	phy_ctrl->rg_pll_lpf_cs = 0;	//0x16[4]
	phy_ctrl->rg_pll_refsel = 1;	//0x16[1:0]

	//test_code_0x1E other parameters config
	phy_ctrl->reload_sel = 1;			//0x1E[4]
	phy_ctrl->rg_phase_gen_en = 1;	//0x1E[3]
	phy_ctrl->pll_power_down = 0;		//0x1E[1]
	phy_ctrl->pll_register_override = 1;	//0x1E[0]

	//HSTX select VCM VREF
	phy_ctrl->rg_vrefsel_vcm = 0x55;
	if (pinfo->mipi.rg_vrefsel_vcm_clk_adjust != 0)
		phy_ctrl->rg_vrefsel_vcm = (phy_ctrl->rg_vrefsel_vcm & 0x0F) |
			((pinfo->mipi.rg_vrefsel_vcm_clk_adjust & 0x0F) << 4);

	if (pinfo->mipi.rg_vrefsel_vcm_data_adjust != 0)
		phy_ctrl->rg_vrefsel_vcm = (phy_ctrl->rg_vrefsel_vcm & 0xF0) |
			(pinfo->mipi.rg_vrefsel_vcm_data_adjust & 0x0F);

	//if reload_sel = 1, need to set load_command
	phy_ctrl->load_command = 0x5A;

	/********************  clock/data lane parameters config  ******************/
	accuracy = 10;
	ui =  10 * 1000000000UL * accuracy / lane_clock;
	//unit of measurement
	unit_tx_byte_clk_hs = 8 * ui;

	// D-PHY Specification : 60ns + 52*UI <= clk_post
	clk_post = 600 * accuracy + 52 * ui + pinfo->mipi.clk_post_adjust * ui;

	// D-PHY Specification : clk_pre >= 8*UI
	clk_pre = 8 * ui + pinfo->mipi.clk_pre_adjust * ui;

	// D-PHY Specification : clk_t_hs_exit >= 100ns
	clk_t_hs_exit = 1000 * accuracy + pinfo->mipi.clk_t_hs_exit_adjust * ui;

	// clocked by TXBYTECLKHS
	clk_pre_delay = 0 + pinfo->mipi.clk_pre_delay_adjust * ui;

	// D-PHY Specification : clk_t_hs_trial >= 60ns
	// clocked by TXBYTECLKHS
	clk_t_hs_trial = 600 * accuracy + 3 * unit_tx_byte_clk_hs + pinfo->mipi.clk_t_hs_trial_adjust * ui;

	// D-PHY Specification : 38ns <= clk_t_hs_prepare <= 95ns
	// clocked by TXBYTECLKHS
	if (pinfo->mipi.clk_t_hs_prepare_adjust == 0)
		pinfo->mipi.clk_t_hs_prepare_adjust = 43;

	clk_t_hs_prepare = ((380 * accuracy + pinfo->mipi.clk_t_hs_prepare_adjust * ui) <= (950 * accuracy - 8 * ui)) ?
		(380 * accuracy + pinfo->mipi.clk_t_hs_prepare_adjust * ui) : (950 * accuracy - 8 * ui);

	// clocked by TXBYTECLKHS
	data_post_delay = 0 + pinfo->mipi.data_post_delay_adjust * ui;

	// D-PHY Specification : data_t_hs_trial >= max( n*8*UI, 60ns + n*4*UI ), n = 1
	// clocked by TXBYTECLKHS
	data_t_hs_trial = ((600 * accuracy + 4 * ui) >= (8 * ui) ? (600 * accuracy + 4 * ui) : (8 * ui)) + 8 * ui +
		3 * unit_tx_byte_clk_hs + pinfo->mipi.data_t_hs_trial_adjust * ui;

	// D-PHY Specification : 40ns + 4*UI <= data_t_hs_prepare <= 85ns + 6*UI
	// clocked by TXBYTECLKHS
	if (pinfo->mipi.data_t_hs_prepare_adjust == 0)
		pinfo->mipi.data_t_hs_prepare_adjust = 35;

	data_t_hs_prepare = ((400  * accuracy + 4 * ui + pinfo->mipi.data_t_hs_prepare_adjust * ui) <= (850 * accuracy + 6 * ui - 8 * ui)) ?
		(400  * accuracy + 4 * ui + pinfo->mipi.data_t_hs_prepare_adjust * ui) : (850 * accuracy + 6 * ui - 8 * ui);

	// D-PHY chip spec : clk_t_lpx + clk_t_hs_prepare > 200ns
	// D-PHY Specification : clk_t_lpx >= 50ns
	// clocked by TXBYTECLKHS
	clk_t_lpx = (((2000 * accuracy - clk_t_hs_prepare) >= 500 * accuracy) ?
		((2000 * accuracy - clk_t_hs_prepare)) : (500 * accuracy)) +
		pinfo->mipi.clk_t_lpx_adjust * ui;

	// D-PHY Specification : clk_t_hs_zero + clk_t_hs_prepare >= 300 ns
	// clocked by TXBYTECLKHS
	clk_t_hs_zero = 3000 * accuracy - clk_t_hs_prepare + 3 * unit_tx_byte_clk_hs + pinfo->mipi.clk_t_hs_zero_adjust * ui;

	// D-PHY chip spec : data_t_lpx + data_t_hs_prepare > 200ns
	// D-PHY Specification : data_t_lpx >= 50ns
	// clocked by TXBYTECLKHS
	data_t_lpx = clk_t_lpx + pinfo->mipi.data_t_lpx_adjust * ui; //2000 * accuracy - data_t_hs_prepare;

	// D-PHY Specification : data_t_hs_zero + data_t_hs_prepare >= 145ns + 10*UI
	// clocked by TXBYTECLKHS
	data_t_hs_zero = 1450 * accuracy + 10 * ui - data_t_hs_prepare +
		3 * unit_tx_byte_clk_hs + pinfo->mipi.data_t_hs_zero_adjust * ui;

	phy_ctrl->clk_pre_delay = ROUND1(clk_pre_delay, unit_tx_byte_clk_hs);
	phy_ctrl->clk_t_hs_prepare = ROUND1(clk_t_hs_prepare, unit_tx_byte_clk_hs);
	phy_ctrl->clk_t_lpx = ROUND1(clk_t_lpx, unit_tx_byte_clk_hs);
	phy_ctrl->clk_t_hs_zero = ROUND1(clk_t_hs_zero, unit_tx_byte_clk_hs);
	phy_ctrl->clk_t_hs_trial = ROUND1(clk_t_hs_trial, unit_tx_byte_clk_hs);

	phy_ctrl->data_post_delay = ROUND1(data_post_delay, unit_tx_byte_clk_hs);
	phy_ctrl->data_t_hs_prepare = ROUND1(data_t_hs_prepare, unit_tx_byte_clk_hs);
	phy_ctrl->data_t_lpx = ROUND1(data_t_lpx, unit_tx_byte_clk_hs);
	phy_ctrl->data_t_hs_zero = ROUND1(data_t_hs_zero, unit_tx_byte_clk_hs);
	phy_ctrl->data_t_hs_trial = ROUND1(data_t_hs_trial, unit_tx_byte_clk_hs);
	phy_ctrl->data_t_ta_go = 4;
	phy_ctrl->data_t_ta_get = 5;

	//
	clk_pre_delay_reality = phy_ctrl->clk_pre_delay + 2;
	clk_t_hs_zero_reality = phy_ctrl->clk_t_hs_zero + 8;
	data_t_hs_zero_reality = phy_ctrl->data_t_hs_zero + 4;
	data_post_delay_reality = phy_ctrl->data_post_delay + 4;

	phy_ctrl->clk_post_delay = phy_ctrl->data_t_hs_trial + ROUND1(clk_post, unit_tx_byte_clk_hs);
	//if use 1080 X 2160 resolution panel,need reduce the lp11 time,and disable noncontinue mode
	if(MIPI_SHORT_LP11 == pinfo->mipi.lp11_flag) {
		phy_ctrl->data_pre_delay = 0;
	} else {
		phy_ctrl->data_pre_delay = clk_pre_delay_reality + phy_ctrl->clk_t_lpx +
			phy_ctrl->clk_t_hs_prepare + clk_t_hs_zero_reality + ROUND1(clk_pre, unit_tx_byte_clk_hs) ;
	}

	//
	clk_post_delay_reality = phy_ctrl->clk_post_delay + 4;

	//if use 1080 X 2160 resolution panel,need reduce the lp11 time,and disable noncontinue mode
	if(MIPI_SHORT_LP11 == pinfo->mipi.lp11_flag) {
		data_pre_delay_reality = 0;
	} else {
		data_pre_delay_reality = phy_ctrl->data_pre_delay + 2;
	}

	phy_ctrl->clk_lane_lp2hs_time = clk_pre_delay_reality + phy_ctrl->clk_t_lpx +
		phy_ctrl->clk_t_hs_prepare + clk_t_hs_zero_reality + 3;
	phy_ctrl->clk_lane_hs2lp_time = clk_post_delay_reality + phy_ctrl->clk_t_hs_trial + 3;
	phy_ctrl->data_lane_lp2hs_time = data_pre_delay_reality + phy_ctrl->data_t_lpx +
		phy_ctrl->data_t_hs_prepare + data_t_hs_zero_reality + 3;
	phy_ctrl->data_lane_hs2lp_time = data_post_delay_reality + phy_ctrl->data_t_hs_trial + 3;

	//if use 1080 X 2160 resolution panel,need reduce the lp11 time,and disable noncontinue mode
	if(MIPI_SHORT_LP11 == pinfo->mipi.lp11_flag) {
		phy_ctrl->phy_stop_wait_time = phy_ctrl->clk_t_hs_trial + ROUND1(clk_t_hs_exit, unit_tx_byte_clk_hs) -
			(data_post_delay_reality + phy_ctrl->data_t_hs_trial);
	} else {
		phy_ctrl->phy_stop_wait_time = clk_post_delay_reality +
			phy_ctrl->clk_t_hs_trial + ROUND1(clk_t_hs_exit, unit_tx_byte_clk_hs) -
			(data_post_delay_reality + phy_ctrl->data_t_hs_trial) + 3;
	}

	phy_ctrl->lane_byte_clk = lane_clock / 8;
	phy_ctrl->clk_division = (((phy_ctrl->lane_byte_clk / 2) % pinfo->mipi.max_tx_esc_clk) > 0) ?
		(phy_ctrl->lane_byte_clk / 2 / pinfo->mipi.max_tx_esc_clk + 1) :
		(phy_ctrl->lane_byte_clk / 2 / pinfo->mipi.max_tx_esc_clk);

	HISI_FB_DEBUG("PHY clock_lane and data_lane config : \n"
		"rg_vrefsel_vcm=%u\n"
		"clk_pre_delay=%u\n"
		"clk_post_delay=%u\n"
		"clk_t_hs_prepare=%u\n"
		"clk_t_lpx=%u\n"
		"clk_t_hs_zero=%u\n"
		"clk_t_hs_trial=%u\n"
		"data_pre_delay=%u\n"
		"data_post_delay=%u\n"
		"data_t_hs_prepare=%u\n"
		"data_t_lpx=%u\n"
		"data_t_hs_zero=%u\n"
		"data_t_hs_trial=%u\n"
		"data_t_ta_go=%u\n"
		"data_t_ta_get=%u\n",
		phy_ctrl->rg_vrefsel_vcm,
		phy_ctrl->clk_pre_delay,
		phy_ctrl->clk_post_delay,
		phy_ctrl->clk_t_hs_prepare,
		phy_ctrl->clk_t_lpx,
		phy_ctrl->clk_t_hs_zero,
		phy_ctrl->clk_t_hs_trial,
		phy_ctrl->data_pre_delay,
		phy_ctrl->data_post_delay,
		phy_ctrl->data_t_hs_prepare,
		phy_ctrl->data_t_lpx,
		phy_ctrl->data_t_hs_zero,
		phy_ctrl->data_t_hs_trial,
		phy_ctrl->data_t_ta_go,
		phy_ctrl->data_t_ta_get);

	HISI_FB_DEBUG("clk_lane_lp2hs_time=%u\n"
		"clk_lane_hs2lp_time=%u\n"
		"data_lane_lp2hs_time=%u\n"
		"data_lane_hs2lp_time=%u\n"
		"phy_stop_wait_time=%u\n",
		phy_ctrl->clk_lane_lp2hs_time,
		phy_ctrl->clk_lane_hs2lp_time,
		phy_ctrl->data_lane_lp2hs_time,
		phy_ctrl->data_lane_hs2lp_time,
		phy_ctrl->phy_stop_wait_time);
}

static void get_dsi_phy_ctrl_v320(struct hisi_fb_data_type *hisifd, struct mipi_dsi_phy_ctrl *phy_ctrl, uint32_t mipi_dsi_bit_clk)
{
	struct hisi_panel_info *pinfo = NULL;
	uint32_t dsi_bit_clk = 0;

	uint32_t ui = 0;
	uint32_t m_pll = 0;
	uint32_t n_pll = 1;
	uint64_t lane_clock = 0;
	uint64_t vco_div = 1;
	uint32_t m_pll_remainder[2] = {0};
	uint32_t i = 0;

	uint32_t accuracy = 0;
	uint32_t unit_tx_byte_clk_hs = 0;
	uint32_t clk_post = 0;
	uint32_t clk_pre =0;
	uint32_t clk_t_hs_exit = 0;
	uint32_t clk_pre_delay = 0;
	uint32_t clk_t_hs_prepare = 0;
	uint32_t clk_t_lpx = 0;
	uint32_t clk_t_hs_zero = 0;
	uint32_t clk_t_hs_trial = 0;
	uint32_t data_post_delay = 0;
	uint32_t data_t_hs_prepare = 0;
	uint32_t data_t_hs_zero = 0;
	uint32_t data_t_hs_trial = 0;
	uint32_t data_t_lpx = 0;
	uint32_t clk_pre_delay_reality = 0;
	uint32_t clk_t_hs_zero_reality = 0;
	uint32_t clk_post_delay_reality = 0;
	uint32_t data_t_hs_zero_reality = 0;
	uint32_t data_post_delay_reality = 0;
	uint32_t data_pre_delay_reality = 0;

	if ((NULL == phy_ctrl) || (NULL == hisifd)) {
		return;
	}
	pinfo = &(hisifd->panel_info);

	//dsi_bit_clk = pinfo->mipi.dsi_bit_clk_upt;
	dsi_bit_clk = mipi_dsi_bit_clk;
	lane_clock = 2 * dsi_bit_clk;
	HISI_FB_DEBUG("Expected : lane_clock = %llu M\n", lane_clock);

	/************************  PLL parameters config  *********************/
	//chip spec :
	//If the output data rate is below 320 Mbps, RG_BNAD_SEL should be set to 1.
	//At this mode a post divider of 1/4 will be applied to VCO.
	if ((320 <= lane_clock) && (lane_clock <= 2500)) {
		phy_ctrl->rg_band_sel = 0;	//0x1E[2]
		vco_div = 1;
	} else if ((80 <= lane_clock) && (lane_clock <320)) {
		phy_ctrl->rg_band_sel = 1;
		vco_div = 4;
	} else {
		HISI_FB_ERR("80M <= lane_clock< = 2500M, not support lane_clock = %llu M\n", lane_clock);
	}

	//if(pinfo->mipi.phy_m_n_count_update) {
	for (i = 0;i < 2;i++) {
		n_pll = i + 1;
		m_pll_remainder[i] = ((n_pll * lane_clock * 1000000UL * 1000UL / DEFAULT_MIPI_CLK_RATE) % 1000) * 10 / 1000;
	}

	n_pll = 1;
	if (m_pll_remainder[1] < m_pll_remainder[0]) {
		n_pll = 2;
	}

	m_pll = n_pll * lane_clock * vco_div * 1000000UL / DEFAULT_MIPI_CLK_RATE;
	HISI_FB_DEBUG("m_pll = %d n_pll  =  %d \n", m_pll,n_pll);

	//phy_ctrl->rg_pll_pre_p = n_pll;

	lane_clock = m_pll * (DEFAULT_MIPI_CLK_RATE / n_pll) / vco_div;
	HISI_FB_DEBUG("Config : lane_clock = %llu\n", lane_clock);

	//test_code_0x14 other parameters config
	//phy_ctrl->rg_pll_enbwt = 0;	//0x14[2]
	phy_ctrl->rg_pll_pre_div1p = n_pll - 1; //0x14[3]
	phy_ctrl->rg_pll_chp = 0;		//0x14[1:0]

	//test_code_0x15 other parameters config
	phy_ctrl->rg_div = m_pll;		//0x15[7:0]

	//test_code_0x16 other parameters config,  0x16[3:2] reserved
	phy_ctrl->rg_pll_cp = 0x3;		//0x16[7:5]
	phy_ctrl->reload_sel = 1;			//0x1E[4]
	phy_ctrl->rg_phase_gen_en = 1;	//0x1E[3]
	phy_ctrl->pll_register_override = 0;	//0x1E[1]
	phy_ctrl->pll_power_down = 1;		//0x1E[0]

	phy_ctrl->rg_lptx_sri = 0x55; //0x13
	if (pinfo->mipi.rg_lptx_sri_adjust != 0)
		phy_ctrl->rg_lptx_sri = pinfo->mipi.rg_lptx_sri_adjust;

	phy_ctrl->rg_vrefsel_lptx = 0x25; //0x1C
	if (pinfo->mipi.rg_vrefsel_lptx_adjust != 0)
		phy_ctrl->rg_vrefsel_lptx = pinfo->mipi.rg_vrefsel_lptx_adjust;

	//HSTX select VCM VREF
	phy_ctrl->rg_vrefsel_vcm = 0x55;      //0x1D
	if (pinfo->mipi.rg_vrefsel_vcm_clk_adjust != 0)
		phy_ctrl->rg_vrefsel_vcm = (phy_ctrl->rg_vrefsel_vcm & 0x0F) |
			((pinfo->mipi.rg_vrefsel_vcm_clk_adjust & 0x0F) << 4);

	if (pinfo->mipi.rg_vrefsel_vcm_data_adjust != 0)
		phy_ctrl->rg_vrefsel_vcm = (phy_ctrl->rg_vrefsel_vcm & 0xF0) |
			(pinfo->mipi.rg_vrefsel_vcm_data_adjust & 0x0F);

	//if reload_sel = 1, need to set load_command
	phy_ctrl->load_command = 0x5A;

	/********************  clock/data lane parameters config  ******************/
	accuracy = 10;
	ui =  10 * 1000000000UL * accuracy / lane_clock;
	//unit of measurement
	unit_tx_byte_clk_hs = 8 * ui;

	// D-PHY Specification : 60ns + 52*UI <= clk_post
	clk_post = 600 * accuracy + 52 * ui + pinfo->mipi.clk_post_adjust * ui;

	// D-PHY Specification : clk_pre >= 8*UI
	clk_pre = 8 * ui + pinfo->mipi.clk_pre_adjust * ui;

	// D-PHY Specification : clk_t_hs_exit >= 100ns
	clk_t_hs_exit = 1000 * accuracy + pinfo->mipi.clk_t_hs_exit_adjust * ui;

	// clocked by TXBYTECLKHS
	clk_pre_delay = 0 + pinfo->mipi.clk_pre_delay_adjust * ui;

	// D-PHY Specification : clk_t_hs_trial >= 60ns
	// clocked by TXBYTECLKHS
	clk_t_hs_trial = 600 * accuracy + 3 * unit_tx_byte_clk_hs + pinfo->mipi.clk_t_hs_trial_adjust * ui;

	// D-PHY Specification : 38ns <= clk_t_hs_prepare <= 95ns
	// clocked by TXBYTECLKHS
	if (pinfo->mipi.clk_t_hs_prepare_adjust == 0)
		pinfo->mipi.clk_t_hs_prepare_adjust = 43;

	clk_t_hs_prepare = ((380 * accuracy + pinfo->mipi.clk_t_hs_prepare_adjust * ui) <= (950 * accuracy - 8 * ui)) ?
		(380 * accuracy + pinfo->mipi.clk_t_hs_prepare_adjust * ui) : (950 * accuracy - 8 * ui);

	// clocked by TXBYTECLKHS
	data_post_delay = 0 + pinfo->mipi.data_post_delay_adjust * ui;

	// D-PHY Specification : data_t_hs_trial >= max( n*8*UI, 60ns + n*4*UI ), n = 1
	// clocked by TXBYTECLKHS
	data_t_hs_trial = ((600 * accuracy + 4 * ui) >= (8 * ui) ? (600 * accuracy + 4 * ui) : (8 * ui)) + 8 * ui +
		3 * unit_tx_byte_clk_hs + pinfo->mipi.data_t_hs_trial_adjust * ui;

	// D-PHY Specification : 40ns + 4*UI <= data_t_hs_prepare <= 85ns + 6*UI
	// clocked by TXBYTECLKHS
	if (pinfo->mipi.data_t_hs_prepare_adjust == 0)
		pinfo->mipi.data_t_hs_prepare_adjust = 35;

	data_t_hs_prepare = ((400  * accuracy + 4 * ui + pinfo->mipi.data_t_hs_prepare_adjust * ui) <= (850 * accuracy + 6 * ui - 8 * ui)) ?
		(400  * accuracy + 4 * ui + pinfo->mipi.data_t_hs_prepare_adjust * ui) : (850 * accuracy + 6 * ui - 8 * ui);

	// D-PHY chip spec : clk_t_lpx + clk_t_hs_prepare > 200ns
	// D-PHY Specification : clk_t_lpx >= 50ns
	// clocked by TXBYTECLKHS
	clk_t_lpx = (((2000 * accuracy - clk_t_hs_prepare) >= 500 * accuracy) ?
		((2000 * accuracy - clk_t_hs_prepare)) : (500 * accuracy)) +
		pinfo->mipi.clk_t_lpx_adjust * ui;

	// D-PHY Specification : clk_t_hs_zero + clk_t_hs_prepare >= 300 ns
	// clocked by TXBYTECLKHS
	clk_t_hs_zero = 3000 * accuracy - clk_t_hs_prepare + 3 * unit_tx_byte_clk_hs + pinfo->mipi.clk_t_hs_zero_adjust * ui;

	// D-PHY chip spec : data_t_lpx + data_t_hs_prepare > 200ns
	// D-PHY Specification : data_t_lpx >= 50ns
	// clocked by TXBYTECLKHS
	data_t_lpx = clk_t_lpx + pinfo->mipi.data_t_lpx_adjust * ui; //2000 * accuracy - data_t_hs_prepare;

	// D-PHY Specification : data_t_hs_zero + data_t_hs_prepare >= 145ns + 10*UI
	// clocked by TXBYTECLKHS
	data_t_hs_zero = 1450 * accuracy + 10 * ui - data_t_hs_prepare +
		3 * unit_tx_byte_clk_hs + pinfo->mipi.data_t_hs_zero_adjust * ui;

	phy_ctrl->clk_pre_delay = ROUND1(clk_pre_delay, unit_tx_byte_clk_hs);
	phy_ctrl->clk_t_hs_prepare = ROUND1(clk_t_hs_prepare, unit_tx_byte_clk_hs);
	phy_ctrl->clk_t_lpx = ROUND1(clk_t_lpx, unit_tx_byte_clk_hs);
	phy_ctrl->clk_t_hs_zero = ROUND1(clk_t_hs_zero, unit_tx_byte_clk_hs);
	phy_ctrl->clk_t_hs_trial = ROUND1(clk_t_hs_trial, unit_tx_byte_clk_hs);

	phy_ctrl->data_post_delay = ROUND1(data_post_delay, unit_tx_byte_clk_hs);
	phy_ctrl->data_t_hs_prepare = ROUND1(data_t_hs_prepare, unit_tx_byte_clk_hs);
	phy_ctrl->data_t_lpx = ROUND1(data_t_lpx, unit_tx_byte_clk_hs);
	phy_ctrl->data_t_hs_zero = ROUND1(data_t_hs_zero, unit_tx_byte_clk_hs);
	phy_ctrl->data_t_hs_trial = ROUND1(data_t_hs_trial, unit_tx_byte_clk_hs);
	phy_ctrl->data_t_ta_go = 4;
	phy_ctrl->data_t_ta_get = 5;

	//
	clk_pre_delay_reality = phy_ctrl->clk_pre_delay + 2;
	clk_t_hs_zero_reality = phy_ctrl->clk_t_hs_zero + 8;
	data_t_hs_zero_reality = phy_ctrl->data_t_hs_zero + 4;
	data_post_delay_reality = phy_ctrl->data_post_delay + 4;

	phy_ctrl->clk_post_delay = phy_ctrl->data_t_hs_trial + ROUND1(clk_post, unit_tx_byte_clk_hs);
	//if use 1080 X 2160 resolution panel,need reduce the lp11 time,and disable noncontinue mode
	if(MIPI_SHORT_LP11 == pinfo->mipi.lp11_flag) {
		phy_ctrl->data_pre_delay = 0;
	} else {
		phy_ctrl->data_pre_delay = clk_pre_delay_reality + phy_ctrl->clk_t_lpx +
			phy_ctrl->clk_t_hs_prepare + clk_t_hs_zero_reality + ROUND1(clk_pre, unit_tx_byte_clk_hs) ;
	}

	//
	clk_post_delay_reality = phy_ctrl->clk_post_delay + 4;

	//if use 1080 X 2160 resolution panel,need reduce the lp11 time,and disable noncontinue mode
	if(MIPI_SHORT_LP11 == pinfo->mipi.lp11_flag) {
		data_pre_delay_reality = 0;
	} else {
		data_pre_delay_reality = phy_ctrl->data_pre_delay + 2;
	}

	phy_ctrl->clk_lane_lp2hs_time = clk_pre_delay_reality + phy_ctrl->clk_t_lpx +
		phy_ctrl->clk_t_hs_prepare + clk_t_hs_zero_reality + 3;
	phy_ctrl->clk_lane_hs2lp_time = clk_post_delay_reality + phy_ctrl->clk_t_hs_trial + 3;
	phy_ctrl->data_lane_lp2hs_time = data_pre_delay_reality + phy_ctrl->data_t_lpx +
		phy_ctrl->data_t_hs_prepare + data_t_hs_zero_reality + 3;
	phy_ctrl->data_lane_hs2lp_time = data_post_delay_reality + phy_ctrl->data_t_hs_trial + 3;

	//if use 1080 X 2160 resolution panel,need reduce the lp11 time,and disable noncontinue mode
	if(MIPI_SHORT_LP11 == pinfo->mipi.lp11_flag) {
		phy_ctrl->phy_stop_wait_time = phy_ctrl->clk_t_hs_trial + ROUND1(clk_t_hs_exit, unit_tx_byte_clk_hs) -
			(data_post_delay_reality + phy_ctrl->data_t_hs_trial);
	} else {
		phy_ctrl->phy_stop_wait_time = clk_post_delay_reality +
			phy_ctrl->clk_t_hs_trial + ROUND1(clk_t_hs_exit, unit_tx_byte_clk_hs) -
			(data_post_delay_reality + phy_ctrl->data_t_hs_trial) + 3;
	}

	phy_ctrl->lane_byte_clk = lane_clock / 8;
	phy_ctrl->clk_division = (((phy_ctrl->lane_byte_clk / 2) % pinfo->mipi.max_tx_esc_clk) > 0) ?
		(phy_ctrl->lane_byte_clk / 2 / pinfo->mipi.max_tx_esc_clk + 1) :
		(phy_ctrl->lane_byte_clk / 2 / pinfo->mipi.max_tx_esc_clk);

	HISI_FB_DEBUG("PHY clock_lane and data_lane config : \n"
		"rg_vrefsel_vcm=%u\n"
		"clk_pre_delay=%u\n"
		"clk_post_delay=%u\n"
		"clk_t_hs_prepare=%u\n"
		"clk_t_lpx=%u\n"
		"clk_t_hs_zero=%u\n"
		"clk_t_hs_trial=%u\n"
		"data_pre_delay=%u\n"
		"data_post_delay=%u\n"
		"data_t_hs_prepare=%u\n"
		"data_t_lpx=%u\n"
		"data_t_hs_zero=%u\n"
		"data_t_hs_trial=%u\n"
		"data_t_ta_go=%u\n"
		"data_t_ta_get=%u\n",
		phy_ctrl->rg_vrefsel_vcm,
		phy_ctrl->clk_pre_delay,
		phy_ctrl->clk_post_delay,
		phy_ctrl->clk_t_hs_prepare,
		phy_ctrl->clk_t_lpx,
		phy_ctrl->clk_t_hs_zero,
		phy_ctrl->clk_t_hs_trial,
		phy_ctrl->data_pre_delay,
		phy_ctrl->data_post_delay,
		phy_ctrl->data_t_hs_prepare,
		phy_ctrl->data_t_lpx,
		phy_ctrl->data_t_hs_zero,
		phy_ctrl->data_t_hs_trial,
		phy_ctrl->data_t_ta_go,
		phy_ctrl->data_t_ta_get);
	HISI_FB_DEBUG("clk_lane_lp2hs_time=%u\n"
		"clk_lane_hs2lp_time=%u\n"
		"data_lane_lp2hs_time=%u\n"
		"data_lane_hs2lp_time=%u\n"
		"phy_stop_wait_time=%u\n",
		phy_ctrl->clk_lane_lp2hs_time,
		phy_ctrl->clk_lane_hs2lp_time,
		phy_ctrl->data_lane_lp2hs_time,
		phy_ctrl->data_lane_hs2lp_time,
		phy_ctrl->phy_stop_wait_time);
}

/*lint +e647*/

/* lint +e834 */
static uint32_t mipi_pixel_clk(struct hisi_fb_data_type *hisifd)
{
	struct hisi_panel_info *pinfo = NULL;
	pinfo = &(hisifd->panel_info);//lint !e838

	if ((pinfo->pxl_clk_rate_div == 0) || (g_fpga_flag == 1)) {
		return (uint32_t)pinfo->pxl_clk_rate;
	}

	if ((pinfo->ifbc_type == IFBC_TYPE_NONE) &&
		!is_dual_mipi_panel(hisifd)) {
		pinfo->pxl_clk_rate_div = 1;
	}

	return (uint32_t)pinfo->pxl_clk_rate / pinfo->pxl_clk_rate_div;
}
/*lint -e715*/
static void mipi_config_phy_test_code(char __iomem *mipi_dsi_base, uint32_t test_code_addr, uint32_t test_code_parameter)
{
	outp32(mipi_dsi_base + MIPIDSI_PHY_TST_CTRL1_OFFSET, test_code_addr);
	outp32(mipi_dsi_base + MIPIDSI_PHY_TST_CTRL0_OFFSET, 0x00000002);
	outp32(mipi_dsi_base + MIPIDSI_PHY_TST_CTRL0_OFFSET, 0x00000000);
	outp32(mipi_dsi_base + MIPIDSI_PHY_TST_CTRL1_OFFSET, test_code_parameter);
	outp32(mipi_dsi_base + MIPIDSI_PHY_TST_CTRL0_OFFSET, 0x00000002);
	outp32(mipi_dsi_base + MIPIDSI_PHY_TST_CTRL0_OFFSET, 0x00000000);
}
/*lint +e715*/

static void mipi_config_dphy_spec_parameter(char __iomem *mipi_dsi_base, struct hisi_panel_info *pinfo)
{
	uint32_t i;
	uint32_t addr = 0;

	//pre_delay of clock lane request setting
	mipi_config_phy_test_code(mipi_dsi_base, MIPIDSI_PHY_TST_CLK_PRE_DELAY, DSS_REDUCE(pinfo->dsi_phy_ctrl.clk_pre_delay));

	//post_delay of clock lane request setting
	mipi_config_phy_test_code(mipi_dsi_base, MIPIDSI_PHY_TST_CLK_POST_DELAY, DSS_REDUCE(pinfo->dsi_phy_ctrl.clk_post_delay));

	//clock lane timing ctrl - t_lpx
	mipi_config_phy_test_code(mipi_dsi_base, MIPIDSI_PHY_TST_CLK_TLPX, DSS_REDUCE(pinfo->dsi_phy_ctrl.clk_t_lpx));

	//clock lane timing ctrl - t_hs_prepare
	mipi_config_phy_test_code(mipi_dsi_base, MIPIDSI_PHY_TST_CLK_PREPARE, DSS_REDUCE(pinfo->dsi_phy_ctrl.clk_t_hs_prepare));

	//clock lane timing ctrl - t_hs_zero
	mipi_config_phy_test_code(mipi_dsi_base, MIPIDSI_PHY_TST_CLK_ZERO, DSS_REDUCE(pinfo->dsi_phy_ctrl.clk_t_hs_zero));

	//clock lane timing ctrl - t_hs_trial
	mipi_config_phy_test_code(mipi_dsi_base, MIPIDSI_PHY_TST_CLK_TRAIL, pinfo->dsi_phy_ctrl.clk_t_hs_trial);

	for (i = 0; i <= pinfo->mipi.lane_nums; i++) {
		//data lane pre_delay
		addr = MIPIDSI_PHY_TST_DATA_PRE_DELAY + (i << 4);
		mipi_config_phy_test_code(mipi_dsi_base, addr, DSS_REDUCE(pinfo->dsi_phy_ctrl.data_pre_delay));

		//data lane post_delay
		addr = MIPIDSI_PHY_TST_DATA_POST_DELAY + (i << 4);
		mipi_config_phy_test_code(mipi_dsi_base, addr, DSS_REDUCE(pinfo->dsi_phy_ctrl.data_post_delay));

		//data lane timing ctrl - t_lpx
		addr = MIPIDSI_PHY_TST_DATA_TLPX + (i << 4);
		mipi_config_phy_test_code(mipi_dsi_base, addr, DSS_REDUCE(pinfo->dsi_phy_ctrl.data_t_lpx));

		//data lane timing ctrl - t_hs_prepare
		addr = MIPIDSI_PHY_TST_DATA_PREPARE + (i << 4);
		mipi_config_phy_test_code(mipi_dsi_base, addr, DSS_REDUCE(pinfo->dsi_phy_ctrl.data_t_hs_prepare));

		//data lane timing ctrl - t_hs_zero
		addr = MIPIDSI_PHY_TST_DATA_ZERO + (i << 4);
		mipi_config_phy_test_code(mipi_dsi_base, addr, DSS_REDUCE(pinfo->dsi_phy_ctrl.data_t_hs_zero));

		//data lane timing ctrl - t_hs_trial
		addr = MIPIDSI_PHY_TST_DATA_TRAIL + (i << 4);
		mipi_config_phy_test_code(mipi_dsi_base, addr, pinfo->dsi_phy_ctrl.data_t_hs_trial);

		//data lane timing ctrl - t_ta_go
		addr = MIPIDSI_PHY_TST_DATA_TA_GO + (i << 4);
		mipi_config_phy_test_code(mipi_dsi_base, addr, DSS_REDUCE(pinfo->dsi_phy_ctrl.data_t_ta_go));

		//data lane timing ctrl - t_ta_get
		addr = MIPIDSI_PHY_TST_DATA_TA_GET + (i << 4);
		mipi_config_phy_test_code(mipi_dsi_base, addr, DSS_REDUCE(pinfo->dsi_phy_ctrl.data_t_ta_get));
	}
}

static void mipi_phy_init_config(struct hisi_fb_data_type *hisifd, char __iomem *mipi_dsi_base, int step)
{
	struct hisi_panel_info *pinfo = NULL;

	pinfo = &(hisifd->panel_info);

	mipi_config_phy_test_code(mipi_dsi_base, 0x00010013, pinfo->dsi_phy_ctrl.rg_lptx_sri);

	if (step == 1) {
		mipi_config_phy_test_code(mipi_dsi_base, 0x00010014,  (1 << 4) + (pinfo->dsi_phy_ctrl.rg_pll_pre_div1p << 3) + 0x0);
		mipi_config_phy_test_code(mipi_dsi_base, 0x00010016, (0x3 << 5) + (pinfo->dsi_phy_ctrl.rg_pll_lpf_cs << 4) + pinfo->dsi_phy_ctrl.rg_pll_refsel);
		HISI_FB_DEBUG("step1: 0x00010014=0x%x, 0x00010016=0x%x\n",  (1 << 4) + (pinfo->dsi_phy_ctrl.rg_pll_pre_div1p << 3) + 0x0, (0x3 << 5) + (pinfo->dsi_phy_ctrl.rg_pll_lpf_cs << 4) + pinfo->dsi_phy_ctrl.rg_pll_refsel);
	} else if (step == 2) {
		mipi_config_phy_test_code(mipi_dsi_base, 0x00010014,  (1 << 4) + (pinfo->dsi_phy_ctrl.rg_pll_pre_div1p << 3) + 0x2);
		mipi_config_phy_test_code(mipi_dsi_base, 0x00010016, (0x7 << 5) + (pinfo->dsi_phy_ctrl.rg_pll_lpf_cs << 4) + pinfo->dsi_phy_ctrl.rg_pll_refsel);
		HISI_FB_DEBUG("step2: 0x00010014=0x%x, 0x00010016=0x%x\n",  (1 << 4) + (pinfo->dsi_phy_ctrl.rg_pll_pre_div1p << 3) + 0x2, (0x7 << 5) + (pinfo->dsi_phy_ctrl.rg_pll_lpf_cs << 4) + pinfo->dsi_phy_ctrl.rg_pll_refsel);
	} else {
		mipi_config_phy_test_code(mipi_dsi_base, 0x00010014,  (1 << 4) + (pinfo->dsi_phy_ctrl.rg_pll_pre_div1p << 3) + pinfo->dsi_phy_ctrl.rg_pll_chp);
		mipi_config_phy_test_code(mipi_dsi_base, 0x00010016, (pinfo->dsi_phy_ctrl.rg_pll_cp << 5) + (pinfo->dsi_phy_ctrl.rg_pll_lpf_cs << 4) + pinfo->dsi_phy_ctrl.rg_pll_refsel);
		HISI_FB_DEBUG("normal: 0x00010014=0x%x, 0x00010016=0x%x\n",  (1 << 4) + (pinfo->dsi_phy_ctrl.rg_pll_pre_div1p << 3) + pinfo->dsi_phy_ctrl.rg_pll_chp, (pinfo->dsi_phy_ctrl.rg_pll_cp << 5) + (pinfo->dsi_phy_ctrl.rg_pll_lpf_cs << 4) + pinfo->dsi_phy_ctrl.rg_pll_refsel);
	}

	//physical configuration PLL II, M
	mipi_config_phy_test_code(mipi_dsi_base, 0x00010015, pinfo->dsi_phy_ctrl.rg_div);

	mipi_config_phy_test_code(mipi_dsi_base, 0x0001001C, pinfo->dsi_phy_ctrl.rg_vrefsel_lptx);

	//sets the analog characteristic of V reference in D-PHY TX
	mipi_config_phy_test_code(mipi_dsi_base, 0x0001001D, pinfo->dsi_phy_ctrl.rg_vrefsel_vcm);

	//MISC AFE Configuration
	mipi_config_phy_test_code(mipi_dsi_base, 0x0001001E, (3 << 5) + (pinfo->dsi_phy_ctrl.reload_sel << 4) +
		(pinfo->dsi_phy_ctrl.rg_phase_gen_en << 3) + (pinfo->dsi_phy_ctrl.rg_band_sel << 2) +
		(pinfo->dsi_phy_ctrl.pll_register_override << 1) + pinfo->dsi_phy_ctrl.pll_power_down);

	//reload_command
	mipi_config_phy_test_code(mipi_dsi_base, 0x0001001F, pinfo->dsi_phy_ctrl.load_command);//5A

}

static void mipi_phy_pll_start_enhance_config(struct hisi_fb_data_type *hisifd, char __iomem *mipi_dsi_base)
{
}

void mipi_init(struct hisi_fb_data_type *hisifd, char __iomem *mipi_dsi_base)
{
	uint32_t hline_time = 0;
	uint32_t hsa_time = 0;
	uint32_t hbp_time = 0;
	uint64_t pixel_clk = 0;
	unsigned long dw_jiffies = 0;
	uint32_t tmp = 0;
	bool is_ready = false;
	struct hisi_panel_info *pinfo = NULL;
	dss_rect_t rect;
	uint32_t cmp_stopstate_val = 0;
	uint32_t mipi_dsi_bit_clk = 0;
	bool pll_start_enhancement = true;

	pinfo = &(hisifd->panel_info);//lint !e838

	if (pinfo->mipi.max_tx_esc_clk == 0) {
		HISI_FB_ERR("fb%d, max_tx_esc_clk is invalid!", hisifd->index);
		pinfo->mipi.max_tx_esc_clk = DEFAULT_MAX_TX_ESC_CLK;
	}

	memset(&(pinfo->dsi_phy_ctrl), 0, sizeof(struct mipi_dsi_phy_ctrl));

	rect.x = 0;
	rect.y = 0;
	rect.w = pinfo->xres;//lint !e713
	rect.h = pinfo->yres;//lint !e713

	mipi_ifbc_get_rect(hisifd, &rect);

	/*************************Configure the PHY start*************************/
	if ((g_dss_version_tag & FB_ACCEL_DSSV320) && (g_fpga_flag == 0)) {
		mipi_dsi_bit_clk = pinfo->mipi.dsi_bit_clk_upt;
		get_dsi_phy_ctrl_v320(hisifd, &(pinfo->dsi_phy_ctrl),mipi_dsi_bit_clk);
	} else {
		get_dsi_phy_ctrl(hisifd, &(pinfo->dsi_phy_ctrl));
	}

	set_reg(mipi_dsi_base + MIPIDSI_PHY_IF_CFG_OFFSET, pinfo->mipi.lane_nums, 2, 0);
	set_reg(mipi_dsi_base + MIPIDSI_CLKMGR_CFG_OFFSET, pinfo->dsi_phy_ctrl.clk_division, 8, 0);
	set_reg(mipi_dsi_base + MIPIDSI_CLKMGR_CFG_OFFSET, pinfo->dsi_phy_ctrl.clk_division, 8, 8);
	outp32(mipi_dsi_base + MIPIDSI_PHY_RSTZ_OFFSET, 0x00000000);
	outp32(mipi_dsi_base + MIPIDSI_PHY_TST_CTRL0_OFFSET, 0x00000000);
	outp32(mipi_dsi_base + MIPIDSI_PHY_TST_CTRL0_OFFSET, 0x00000001);
	outp32(mipi_dsi_base + MIPIDSI_PHY_TST_CTRL0_OFFSET, 0x00000000);

	if ((g_dss_version_tag & FB_ACCEL_DSSV320) && (g_fpga_flag == 0)) {
		mipi_config_phy_test_code(mipi_dsi_base, 0x00010012, 0x00000020);
		mipi_config_phy_test_code(mipi_dsi_base, 0x00010012, 0x00000030);

		if (pll_start_enhancement) {
			mipi_phy_pll_start_enhance_config(hisifd, mipi_dsi_base);
		} else {
			mipi_phy_init_config(hisifd, mipi_dsi_base, 0);
		}
		//set dphy spec parameter
		mipi_config_dphy_spec_parameter(mipi_dsi_base, pinfo);
	} else {

		//physical configuration PLL I
		mipi_config_phy_test_code(mipi_dsi_base, 0x00010014, (pinfo->dsi_phy_ctrl.rg_pll_fbd_s << 4) +
			(pinfo->dsi_phy_ctrl.rg_pll_enswc << 3) + (pinfo->dsi_phy_ctrl.rg_pll_enbwt << 2) + pinfo->dsi_phy_ctrl.rg_pll_chp);

		//physical configuration PLL II, M
		mipi_config_phy_test_code(mipi_dsi_base, 0x00010015, pinfo->dsi_phy_ctrl.rg_pll_fbd_p);

		//physical configuration PLL III
		mipi_config_phy_test_code(mipi_dsi_base, 0x00010016, (pinfo->dsi_phy_ctrl.rg_pll_cp << 5) +
			(pinfo->dsi_phy_ctrl.rg_pll_lpf_cs << 4) + pinfo->dsi_phy_ctrl.rg_pll_refsel);

		//physical configuration PLL IV, N
		mipi_config_phy_test_code(mipi_dsi_base, 0x00010017, pinfo->dsi_phy_ctrl.rg_pll_pre_p);

		//sets the analog characteristic of V reference in D-PHY TX
		mipi_config_phy_test_code(mipi_dsi_base, 0x0001001D, pinfo->dsi_phy_ctrl.rg_vrefsel_vcm);

		//MISC AFE Configuration
		mipi_config_phy_test_code(mipi_dsi_base, 0x0001001E, (pinfo->dsi_phy_ctrl.rg_pll_cp_p << 5) +
			(pinfo->dsi_phy_ctrl.reload_sel << 4) + (pinfo->dsi_phy_ctrl.rg_phase_gen_en << 3) +
			(pinfo->dsi_phy_ctrl.rg_band_sel << 2) + (pinfo->dsi_phy_ctrl.pll_power_down << 1) +
			pinfo->dsi_phy_ctrl.pll_register_override);

		//reload_command
		mipi_config_phy_test_code(mipi_dsi_base, 0x0001001F, pinfo->dsi_phy_ctrl.load_command);

		//set LPTX analog test_mode
		mipi_config_phy_test_code(mipi_dsi_base, 0x00010012, 0x00000030);

		//set dphy spec parameter
		mipi_config_dphy_spec_parameter(mipi_dsi_base, pinfo);

	}

	outp32(mipi_dsi_base + MIPIDSI_PHY_RSTZ_OFFSET, 0x0000000F);

	is_ready = false;
	dw_jiffies = jiffies + HZ / 2;
	do {
		tmp = inp32(mipi_dsi_base + MIPIDSI_PHY_STATUS_OFFSET);
		if ((tmp & 0x00000001) == 0x00000001) {
			is_ready = true;
			break;
		}
	} while (time_after(dw_jiffies, jiffies));

	if (!is_ready) {
		HISI_FB_INFO("fb%d, phylock is not ready!MIPIDSI_PHY_STATUS_OFFSET=0x%x.\n",
			hisifd->index, tmp);
	}

	if (pinfo->mipi.lane_nums >= DSI_4_LANES) {
		cmp_stopstate_val = (BIT(4) | BIT(7) | BIT(9) | BIT(11));
	} else if (pinfo->mipi.lane_nums >= DSI_3_LANES) {
		cmp_stopstate_val = (BIT(4) | BIT(7) | BIT(9));
	} else if (pinfo->mipi.lane_nums >= DSI_2_LANES) {
		cmp_stopstate_val = (BIT(4) | BIT(7));
	} else {
		cmp_stopstate_val = (BIT(4));
	}

	is_ready = false;
	dw_jiffies = jiffies + HZ / 2;
	do {
		tmp = inp32(mipi_dsi_base + MIPIDSI_PHY_STATUS_OFFSET);
		if ((tmp & cmp_stopstate_val) == cmp_stopstate_val) {
			is_ready = true;
			break;
		}
	} while (time_after(dw_jiffies, jiffies));

	if (!is_ready) {
		HISI_FB_INFO("fb%d, phystopstateclklane is not ready! MIPIDSI_PHY_STATUS_OFFSET=0x%x.\n",hisifd->index, tmp);
	}

	/*************************Configure the PHY end*************************/

	if (is_mipi_cmd_panel(hisifd)) {
		// config to command mode
		set_reg(mipi_dsi_base + MIPIDSI_MODE_CFG_OFFSET, 0x1, 1, 0);
		// ALLOWED_CMD_SIZE
		set_reg(mipi_dsi_base + MIPIDSI_EDPI_CMD_SIZE_OFFSET, rect.w, 16, 0);

		// cnt=2 in update-patial scene, cnt nees to be checked for different panels
		if (pinfo->mipi.hs_wr_to_time == 0) {
			set_reg(mipi_dsi_base + MIPIDSI_HS_WR_TO_CNT_OFFSET, 0x1000002, 25, 0);
		} else {
			set_reg(mipi_dsi_base + MIPIDSI_HS_WR_TO_CNT_OFFSET,
				(0x1 << 24) | (pinfo->mipi.hs_wr_to_time * pinfo->dsi_phy_ctrl.lane_byte_clk / 1000000000UL), 25, 0);
		}

		// FIXME: test tearing effect, if use gpio, no need
		//set_reg(mipi_dsi_base + MIPIDSI_CMD_MODE_CFG_OFFSET, 0x1, 1, 0);
	}

	// phy_stop_wait_time
	set_reg(mipi_dsi_base + MIPIDSI_PHY_IF_CFG_OFFSET, pinfo->dsi_phy_ctrl.phy_stop_wait_time, 8, 8);

	//--------------configuring the DPI packet transmission----------------
	/*
	** 2. Configure the DPI Interface:
	** This defines how the DPI interface interacts with the controller.
	*/
	set_reg(mipi_dsi_base + MIPIDSI_DPI_VCID_OFFSET, pinfo->mipi.vc, 2, 0);
	set_reg(mipi_dsi_base + MIPIDSI_DPI_COLOR_CODING_OFFSET, pinfo->mipi.color_mode, 4, 0);

	set_reg(mipi_dsi_base + MIPIDSI_DPI_CFG_POL_OFFSET, pinfo->ldi.data_en_plr, 1, 0);
	set_reg(mipi_dsi_base + MIPIDSI_DPI_CFG_POL_OFFSET, pinfo->ldi.vsync_plr, 1, 1);
	set_reg(mipi_dsi_base + MIPIDSI_DPI_CFG_POL_OFFSET, pinfo->ldi.hsync_plr, 1, 2);
	set_reg(mipi_dsi_base + MIPIDSI_DPI_CFG_POL_OFFSET, 0x0, 1, 3);
	set_reg(mipi_dsi_base + MIPIDSI_DPI_CFG_POL_OFFSET, 0x0, 1, 4);


	/*
	** 3. Select the Video Transmission Mode:
	** This defines how the processor requires the video line to be
	** transported through the DSI link.
	*/
	// video mode: low power mode
	if (MIPI_DISABLE_LP11 == pinfo->mipi.lp11_flag) {
		set_reg(mipi_dsi_base + MIPIDSI_VID_MODE_CFG_OFFSET, 0x0f, 6, 8);
		HISI_FB_INFO("set_reg MIPIDSI_VID_MODE_CFG_OFFSET 0x0f \n");
	} else {
		set_reg(mipi_dsi_base + MIPIDSI_VID_MODE_CFG_OFFSET, 0x3f, 6, 8);
		HISI_FB_INFO("set_reg MIPIDSI_VID_MODE_CFG_OFFSET 0x3f \n");
	}
	/* set_reg(mipi_dsi_base + MIPIDSI_VID_MODE_CFG_OFFSET, 0x0, 1, 14); */
	if (is_mipi_video_panel(hisifd)) {
		// TODO: fix blank display bug when set backlight
		set_reg(mipi_dsi_base + MIPIDSI_DPI_LP_CMD_TIM_OFFSET, 0x4, 8, 16);
		// video mode: send read cmd by lp mode
		set_reg(mipi_dsi_base + MIPIDSI_VID_MODE_CFG_OFFSET, 0x1, 1, 15);
	}

	if ((pinfo->mipi.dsi_version == DSI_1_2_VERSION)
		&& (is_mipi_video_panel(hisifd))
		&& ((pinfo->ifbc_type == IFBC_TYPE_VESA3X_SINGLE)
			|| (pinfo->ifbc_type == IFBC_TYPE_VESA3X_DUAL))) {

		set_reg(mipi_dsi_base + MIPIDSI_VID_PKT_SIZE_OFFSET, rect.w * pinfo->pxl_clk_rate_div, 14, 0);
		// video vase3x must be set BURST mode
		if (pinfo->mipi.burst_mode < DSI_BURST_SYNC_PULSES_1) {
			HISI_FB_INFO("pinfo->mipi.burst_mode = %d. video need config BURST mode\n",pinfo->mipi.burst_mode);
			pinfo->mipi.burst_mode = DSI_BURST_SYNC_PULSES_1;
		}
	} else {
		set_reg(mipi_dsi_base + MIPIDSI_VID_PKT_SIZE_OFFSET, rect.w, 14, 0);
	}

	// burst mode
	set_reg(mipi_dsi_base + MIPIDSI_VID_MODE_CFG_OFFSET, pinfo->mipi.burst_mode, 2, 0);
	// for dsi read, BTA enable
	set_reg(mipi_dsi_base + MIPIDSI_PCKHDL_CFG_OFFSET, 0x1, 1, 2);

	/*
	** 4. Define the DPI Horizontal timing configuration:
	**
	** Hsa_time = HSA*(PCLK period/Clk Lane Byte Period);
	** Hbp_time = HBP*(PCLK period/Clk Lane Byte Period);
	** Hline_time = (HSA+HBP+HACT+HFP)*(PCLK period/Clk Lane Byte Period);
	*/
	pixel_clk = mipi_pixel_clk(hisifd);
	HISI_FB_INFO("pixel_clk = %llu\n", pixel_clk);

/*lint -e737 -e776 -e712*/
	if (pinfo->mipi.phy_mode == DPHY_MODE) {
		hsa_time = pinfo->ldi.h_pulse_width * pinfo->dsi_phy_ctrl.lane_byte_clk / pixel_clk;
		hbp_time = pinfo->ldi.h_back_porch * pinfo->dsi_phy_ctrl.lane_byte_clk / pixel_clk;
		hline_time = (pinfo->ldi.h_pulse_width + pinfo->ldi.h_back_porch +
			rect.w + pinfo->ldi.h_front_porch) * pinfo->dsi_phy_ctrl.lane_byte_clk / pixel_clk;
	} else {
		hsa_time = pinfo->ldi.h_pulse_width * pinfo->dsi_phy_ctrl.lane_word_clk / pixel_clk;
		hbp_time = pinfo->ldi.h_back_porch * pinfo->dsi_phy_ctrl.lane_word_clk / pixel_clk;
		hline_time = (pinfo->ldi.h_pulse_width + pinfo->ldi.h_back_porch +
			rect.w + pinfo->ldi.h_front_porch) * pinfo->dsi_phy_ctrl.lane_word_clk / pixel_clk;
	}
	HISI_FB_INFO("hsa_time = %d, hbp_time = %d, hline_time = %d \n", hsa_time, hbp_time, hline_time);
/*lint +e737 +e776 +e712*/

	set_reg(mipi_dsi_base + MIPIDSI_VID_HSA_TIME_OFFSET, hsa_time, 12, 0);
	set_reg(mipi_dsi_base + MIPIDSI_VID_HBP_TIME_OFFSET, hbp_time, 12, 0);
	set_reg(mipi_dsi_base + MIPIDSI_VID_HLINE_TIME_OFFSET, hline_time, 15, 0);

	// Define the Vertical line configuration
	set_reg(mipi_dsi_base + MIPIDSI_VID_VSA_LINES_OFFSET, pinfo->ldi.v_pulse_width, 10, 0);
	set_reg(mipi_dsi_base + MIPIDSI_VID_VBP_LINES_OFFSET, pinfo->ldi.v_back_porch, 10, 0);
	set_reg(mipi_dsi_base + MIPIDSI_VID_VFP_LINES_OFFSET, pinfo->ldi.v_front_porch, 10, 0);
	set_reg(mipi_dsi_base + MIPIDSI_VID_VACTIVE_LINES_OFFSET, rect.h, 14, 0);
	set_reg(mipi_dsi_base + MIPIDSI_TO_CNT_CFG_OFFSET, 0x7FF, 16, 0);

	// Configure core's phy parameters
	set_reg(mipi_dsi_base + MIPIDSI_PHY_TMR_LPCLK_CFG_OFFSET, pinfo->dsi_phy_ctrl.clk_lane_lp2hs_time, 10, 0);
	set_reg(mipi_dsi_base + MIPIDSI_PHY_TMR_LPCLK_CFG_OFFSET, pinfo->dsi_phy_ctrl.clk_lane_hs2lp_time, 10, 16);
	set_reg(mipi_dsi_base + MIPIDSI_PHY_TMR_RD_CFG_OFFSET, 0x7FFF, 15, 0);
	set_reg(mipi_dsi_base + MIPIDSI_PHY_TMR_CFG_OFFSET, pinfo->dsi_phy_ctrl.data_lane_lp2hs_time, 10, 0);
	set_reg(mipi_dsi_base + MIPIDSI_PHY_TMR_CFG_OFFSET, pinfo->dsi_phy_ctrl.data_lane_hs2lp_time, 10, 16);

	// Waking up Core
	set_reg(mipi_dsi_base + MIPIDSI_PWR_UP_OFFSET, 0x1, 1, 0);
}

int mipi_dsi_clk_enable(struct hisi_fb_data_type *hisifd)
{
	int ret = 0;
	struct clk *clk_tmp = NULL;

	if (NULL == hisifd) {
		HISI_FB_ERR("hisifd is NULL");
		return -EINVAL;
	}

	if (hisifd->index == PRIMARY_PANEL_IDX) {
		clk_tmp = hisifd->dss_dphy0_ref_clk;
		if (clk_tmp) {
			ret = clk_prepare(clk_tmp);
			if (ret) {
				HISI_FB_ERR("fb%d dss_dphy0_ref_clk clk_prepare failed, error=%d!\n",
					hisifd->index, ret);
				return -EINVAL;
			}

			ret = clk_enable(clk_tmp);
			if (ret) {
				HISI_FB_ERR("fb%d dss_dphy0_ref_clk clk_enable failed, error=%d!\n",
					hisifd->index, ret);
				return -EINVAL;
			}
		}

		clk_tmp = hisifd->dss_dphy0_cfg_clk;
		if (clk_tmp) {
			ret = clk_prepare(clk_tmp);
			if (ret) {
				HISI_FB_ERR("fb%d dss_dphy0_cfg_clk clk_prepare failed, error=%d!\n",
					hisifd->index, ret);
				return -EINVAL;
			}

			ret = clk_enable(clk_tmp);
			if (ret) {
				HISI_FB_ERR("fb%d dss_dphy0_cfg_clk clk_enable failed, error=%d!\n",
					hisifd->index, ret);
				return -EINVAL;
			}
		}

		clk_tmp = hisifd->dss_pclk_dsi0_clk;
		if (clk_tmp) {
			ret = clk_prepare(clk_tmp);
			if (ret) {
				HISI_FB_ERR("fb%d dss_pclk_dsi0_clk clk_prepare failed, error=%d!\n",
					hisifd->index, ret);
				return -EINVAL;
			}

			ret = clk_enable(clk_tmp);
			if (ret) {
				HISI_FB_ERR("fb%d dss_pclk_dsi0_clk clk_enable failed, error=%d!\n",
					hisifd->index, ret);
				return -EINVAL;
			}
		}
	}


	if (is_dual_mipi_panel(hisifd) || (hisifd->index == EXTERNAL_PANEL_IDX)) {
		clk_tmp = hisifd->dss_dphy1_ref_clk;
		if (clk_tmp) {
			ret = clk_prepare(clk_tmp);
			if (ret) {
				HISI_FB_ERR("fb%d dss_dphy1_ref_clk clk_prepare failed, error=%d!\n",
					hisifd->index, ret);
				return -EINVAL;
			}

			ret = clk_enable(clk_tmp);
			if (ret) {
				HISI_FB_ERR("fb%d dss_dphy1_ref_clk clk_enable failed, error=%d!\n",
					hisifd->index, ret);
				return -EINVAL;
			}
		}

		clk_tmp = hisifd->dss_dphy1_cfg_clk;
		if (clk_tmp) {
			ret = clk_prepare(clk_tmp);
			if (ret) {
				HISI_FB_ERR("fb%d dss_dphy1_cfg_clk clk_prepare failed, error=%d!\n",
					hisifd->index, ret);
				return -EINVAL;
			}

			ret = clk_enable(clk_tmp);
			if (ret) {
				HISI_FB_ERR("fb%d dss_dphy1_cfg_clk clk_enable failed, error=%d!\n",
					hisifd->index, ret);
				return -EINVAL;
			}
		}

		clk_tmp = hisifd->dss_pclk_dsi1_clk;
		if (clk_tmp) {
			ret = clk_prepare(clk_tmp);
			if (ret) {
				HISI_FB_ERR("fb%d dss_pclk_dsi1_clk clk_prepare failed, error=%d!\n",
					hisifd->index, ret);
				return -EINVAL;
			}

			ret = clk_enable(clk_tmp);
			if (ret) {
				HISI_FB_ERR("fb%d dss_pclk_dsi1_clk clk_enable failed, error=%d!\n",
					hisifd->index, ret);
				return -EINVAL;
			}
		}
	}

	return 0;
}

int mipi_dsi_clk_disable(struct hisi_fb_data_type *hisifd)
{
	struct clk *clk_tmp = NULL;

	if (NULL == hisifd) {
		HISI_FB_ERR("hisifd is NULL");
		return -EINVAL;
	}

	if (hisifd->index == PRIMARY_PANEL_IDX) {
		clk_tmp = hisifd->dss_dphy0_ref_clk;
		if (clk_tmp) {
			clk_disable(clk_tmp);
			clk_unprepare(clk_tmp);
		}

		clk_tmp = hisifd->dss_dphy0_cfg_clk;
		if (clk_tmp) {
			clk_disable(clk_tmp);
			clk_unprepare(clk_tmp);
		}

		clk_tmp = hisifd->dss_pclk_dsi0_clk;
		if (clk_tmp) {
			clk_disable(clk_tmp);
			clk_unprepare(clk_tmp);
		}
	}


	if (is_dual_mipi_panel(hisifd) || (hisifd->index == EXTERNAL_PANEL_IDX)) {
		clk_tmp = hisifd->dss_dphy1_ref_clk;
		if (clk_tmp) {
			clk_disable(clk_tmp);
			clk_unprepare(clk_tmp);
		}

		clk_tmp = hisifd->dss_dphy1_cfg_clk;
		if (clk_tmp) {
			clk_disable(clk_tmp);
			clk_unprepare(clk_tmp);
		}

		clk_tmp = hisifd->dss_pclk_dsi1_clk;
		if (clk_tmp) {
			clk_disable(clk_tmp);
			clk_unprepare(clk_tmp);
		}
	}

	return 0;
}


/*******************************************************************************
**
*/
/*lint -e732*/
static int mipi_dsi_pll_status_check_ec(struct hisi_fb_data_type *hisifd,
	char __iomem *mipi_dsi_base)
{
	uint32_t tmp;
	uint32_t cmp_ulpsactivenot_val = 0;
	uint32_t cmp_stopstate_val = 0;
	uint32_t try_times;
	struct timeval tv0;
	struct timeval tv1;
	uint32_t redo_count = 0;

	if (NULL == hisifd || NULL == mipi_dsi_base) {
		HISI_FB_ERR("hisifd or mipi_dsi_base is null.\n");
		return 0;
	}

	HISI_FB_DEBUG("fb%d, +!\n", hisifd->index);

	hisifb_get_timestamp(&tv0);

	////////////////////////////////////////////////////////////////////////////
	//
	// enter ulps
	//
	if (hisifd->panel_info.mipi.lane_nums >= DSI_4_LANES) {
		cmp_ulpsactivenot_val = (BIT(5) | BIT(8) | BIT(10) | BIT(12));
		cmp_stopstate_val = (BIT(4) | BIT(7) | BIT(9) | BIT(11));
	} else if (hisifd->panel_info.mipi.lane_nums >= DSI_3_LANES) {
		cmp_ulpsactivenot_val = (BIT(5) | BIT(8) | BIT(10));
		cmp_stopstate_val = (BIT(4) | BIT(7) | BIT(9));
	} else if (hisifd->panel_info.mipi.lane_nums >= DSI_2_LANES) {
		cmp_ulpsactivenot_val = (BIT(5) | BIT(8));
		cmp_stopstate_val = (BIT(4) | BIT(7));
	} else {
		cmp_ulpsactivenot_val = (BIT(5));
		cmp_stopstate_val = (BIT(4));
	}

REDO:
	if (redo_count > 100) {
		HISI_FB_ERR("mipi phy pll retry fail, phy is not ready.\n");
		return 0;
	}
	redo_count ++;

	// check DPHY data and clock lane stopstate
	try_times = 0;
	tmp = inp32(mipi_dsi_base + MIPIDSI_PHY_STATUS_OFFSET);
	while ((tmp & cmp_stopstate_val) != cmp_stopstate_val) {
		udelay(10); //lint !e774  !e747  !e778
		if (++try_times > 100) {
			HISI_FB_INFO("fb%d, check1, check phy data clk lane stop state fail, PHY_STATUS=0x%x!\n",
				hisifd->index, tmp);

			set_reg(mipi_dsi_base + MIPIDSI_PHY_ULPS_CTRL_OFFSET, 0x0, 32, 0);
			set_reg(mipi_dsi_base + MIPIDSI_PHY_RSTZ_OFFSET, 0x0, 1, 0);
			udelay(5); //lint !e774  !e747  !e778
			mipi_phy_pll_start_enhance_config(hisifd, mipi_dsi_base);
			set_reg(mipi_dsi_base + MIPIDSI_PHY_RSTZ_OFFSET, 0x1, 1, 0);
			goto REDO;
		}

		tmp = inp32(mipi_dsi_base + MIPIDSI_PHY_STATUS_OFFSET); //lint !e732
	}

	// disable DPHY clock lane's Hight Speed Clock
	//set_reg(mipi_dsi_base + MIPIDSI_LPCLK_CTRL_OFFSET, 0x0, 1, 0);

	// request that data lane enter ULPS
	set_reg(mipi_dsi_base + MIPIDSI_PHY_ULPS_CTRL_OFFSET, 0x4, 4, 0);

	//check DPHY data lane ulpsactivenot_status
	try_times = 0;
	tmp = inp32(mipi_dsi_base + MIPIDSI_PHY_STATUS_OFFSET); //lint !e732
	while ((tmp & cmp_ulpsactivenot_val) != 0) {
		if (++try_times > 4) {
			HISI_FB_INFO("fb%d, check2, request data lane enter ulps fail, PHY_STATUS=0x%x!\n",
				hisifd->index, tmp);

			set_reg(mipi_dsi_base + MIPIDSI_PHY_ULPS_CTRL_OFFSET, 0x0, 32, 0);
			set_reg(mipi_dsi_base + MIPIDSI_PHY_RSTZ_OFFSET, 0x0, 1, 0);
			udelay(5); //lint !e774  !e747  !e778
			mipi_phy_pll_start_enhance_config(hisifd, mipi_dsi_base);
			set_reg(mipi_dsi_base + MIPIDSI_PHY_RSTZ_OFFSET, 0x1, 1, 0);
			goto REDO;
		}

		udelay(5); //lint !e774  !e747  !e778
		tmp = inp32(mipi_dsi_base + MIPIDSI_PHY_STATUS_OFFSET); //lint !e732
	}

	// enable DPHY PLL, force_pll = 1
	outp32(mipi_dsi_base + MIPIDSI_PHY_RSTZ_OFFSET, 0xF);

	////////////////////////////////////////////////////////////////////////////
	//
	// exit ulps
	//
	// request that data lane  exit ULPS
	outp32(mipi_dsi_base + MIPIDSI_PHY_ULPS_CTRL_OFFSET, 0xC);
	try_times= 0;
	tmp = inp32(mipi_dsi_base + MIPIDSI_PHY_STATUS_OFFSET); //lint !e732
	while ((tmp & cmp_ulpsactivenot_val) != cmp_ulpsactivenot_val) {
		udelay(10); //lint !e774  !e747  !e778
		if (++try_times > 3) {
			HISI_FB_INFO("fb%d, check3, request data clock lane exit ulps fail, PHY_STATUS=0x%x!\n",
				hisifd->index, tmp);

			set_reg(mipi_dsi_base + MIPIDSI_PHY_ULPS_CTRL_OFFSET, 0x0, 32, 0);
			set_reg(mipi_dsi_base + MIPIDSI_PHY_RSTZ_OFFSET, 0x0, 1, 0);
			udelay(5); //lint !e774  !e747  !e778
			mipi_phy_pll_start_enhance_config(hisifd, mipi_dsi_base);
			set_reg(mipi_dsi_base + MIPIDSI_PHY_RSTZ_OFFSET, 0x1, 1, 0);
			goto REDO;
		}

		tmp = inp32(mipi_dsi_base + MIPIDSI_PHY_STATUS_OFFSET); //lint !e732
	}

	// clear PHY_ULPS_CTRL
	outp32(mipi_dsi_base + MIPIDSI_PHY_ULPS_CTRL_OFFSET, 0x0);

	try_times= 0;
	tmp = inp32(mipi_dsi_base + MIPIDSI_PHY_STATUS_OFFSET); //lint !e732
	while ((tmp & cmp_stopstate_val) != cmp_stopstate_val) {
		udelay(10); //lint !e774  !e747  !e778
		if (++try_times > 3) {
			HISI_FB_INFO("fb%d, check4, wait data clock lane stop state fail, PHY_STATUS=0x%x!\n",
				hisifd->index, tmp);

			set_reg(mipi_dsi_base + MIPIDSI_PHY_ULPS_CTRL_OFFSET, 0x0, 32, 0);
			set_reg(mipi_dsi_base + MIPIDSI_PHY_RSTZ_OFFSET, 0x0, 1, 0);
			udelay(5); //lint !e774  !e747  !e778
			mipi_phy_pll_start_enhance_config(hisifd, mipi_dsi_base);
			set_reg(mipi_dsi_base + MIPIDSI_PHY_RSTZ_OFFSET, 0x1, 1, 0);
			goto REDO;
		}

		tmp = inp32(mipi_dsi_base + MIPIDSI_PHY_STATUS_OFFSET); //lint !e732
	}

	//set LPTX analog normal_mode
	mipi_config_phy_test_code(mipi_dsi_base, 0x00010012, 0x00000000);



	// enable DPHY clock lane's Hight Speed Clock
	//set_reg(mipi_dsi_base + MIPIDSI_LPCLK_CTRL_OFFSET, 0x1, 1, 0);

	hisifb_get_timestamp(&tv1);

	HISI_FB_INFO("fb%d, wait data clock lane stop state SUCC, redo_count=%d, cost time %u us!\n",
		hisifd->index, redo_count, hisifb_timestamp_diff(&tv0, &tv1));

	HISI_FB_DEBUG("fb%d, -!\n", hisifd->index);

	return 0;
}
/*lint +e732*/
/*lint -e776 -e715 -e712 -e737 -e776 -e838*/
static int mipi_dsi_on_sub1(struct hisi_fb_data_type *hisifd, char __iomem *mipi_dsi_base)
{
	if (NULL == hisifd || NULL == mipi_dsi_base) {
		HISI_FB_ERR("hisifd or mipi_dsi_base is null.\n");
		return 0;
	}

	/* mipi init */
	mipi_init(hisifd, mipi_dsi_base);

	/* dsi memory init */
	if (g_dss_version_tag == FB_ACCEL_KIRIN970) {
		outp32(mipi_dsi_base + DSI_MEM_CTRL, 0x02600008);
	}

	/* switch to cmd mode */
	set_reg(mipi_dsi_base + MIPIDSI_MODE_CFG_OFFSET, 0x1, 1, 0);
	/* cmd mode: low power mode */
	set_reg(mipi_dsi_base + MIPIDSI_CMD_MODE_CFG_OFFSET, 0x7f, 7, 8);
	set_reg(mipi_dsi_base + MIPIDSI_CMD_MODE_CFG_OFFSET, 0xf, 4, 16);
	set_reg(mipi_dsi_base + MIPIDSI_CMD_MODE_CFG_OFFSET, 0x1, 1, 24);
	/* disable generate High Speed clock */
	/* delete? */
	set_reg(mipi_dsi_base + MIPIDSI_LPCLK_CTRL_OFFSET, 0x0, 1, 0);

	return 0;
}

static void pctrl_dphytx_stopcnt_config(struct hisi_fb_data_type *hisifd)
{
	struct hisi_panel_info *pinfo = NULL;
	uint64_t pctrl_dphytx_stopcnt = 0;
	uint32_t stopcnt_div = 1;

	if (NULL == hisifd) {
		HISI_FB_ERR("hisifd or mipi_dsi_base is null.\n");
		return;
	}
	pinfo = &(hisifd->panel_info);

	stopcnt_div = is_dual_mipi_panel(hisifd) ? 2 : 1;
	// init: wait DPHY 4 data lane stopstate
	if (is_mipi_video_panel(hisifd)) {
		pctrl_dphytx_stopcnt = (uint64_t)(pinfo->ldi.h_back_porch +
			pinfo->ldi.h_front_porch + pinfo->ldi.h_pulse_width + pinfo->xres / stopcnt_div + 5) *
			hisifd->dss_vote_cmd.dss_pclk_pctrl_rate / (pinfo->pxl_clk_rate / stopcnt_div);
	} else {
		pctrl_dphytx_stopcnt = (uint64_t)(pinfo->ldi.h_back_porch +
			pinfo->ldi.h_front_porch + pinfo->ldi.h_pulse_width + 5) *
			hisifd->dss_vote_cmd.dss_pclk_pctrl_rate / (pinfo->pxl_clk_rate / stopcnt_div);
	}
	//FIXME:
	outp32(hisifd->pctrl_base + PERI_CTRL29, (uint32_t)pctrl_dphytx_stopcnt);
	if (is_dual_mipi_panel(hisifd)) {
		outp32(hisifd->pctrl_base + PERI_CTRL32, (uint32_t)pctrl_dphytx_stopcnt);
	}

	return;
}

static int mipi_dsi_on_sub2(struct hisi_fb_data_type *hisifd, char __iomem *mipi_dsi_base)
{
	struct hisi_panel_info *pinfo = NULL;

	if (NULL == hisifd || NULL == mipi_dsi_base) {
		HISI_FB_ERR("hisifd or mipi_dsi_base is null.\n");
		return 0;
	}

	pinfo = &(hisifd->panel_info);

	if (is_mipi_video_panel(hisifd)) {
		/* switch to video mode */
		set_reg(mipi_dsi_base + MIPIDSI_MODE_CFG_OFFSET, 0x0, 1, 0);
	}

	if (is_mipi_cmd_panel(hisifd)) {
		/* cmd mode: high speed mode */
		set_reg(mipi_dsi_base + MIPIDSI_CMD_MODE_CFG_OFFSET, 0x0, 7, 8);
		set_reg(mipi_dsi_base + MIPIDSI_CMD_MODE_CFG_OFFSET, 0x0, 4, 16);
		set_reg(mipi_dsi_base + MIPIDSI_CMD_MODE_CFG_OFFSET, 0x0, 1, 24);
	}

	/* enable EOTP TX */
	if (pinfo->mipi.phy_mode == DPHY_MODE) {
		/* Some vendors don't need eotp check.*/
		if(pinfo->mipi.eotp_disable_flag == 1){
		    set_reg(mipi_dsi_base + MIPIDSI_PCKHDL_CFG_OFFSET, 0x0, 1, 0);
		} else {
		    set_reg(mipi_dsi_base + MIPIDSI_PCKHDL_CFG_OFFSET, 0x1, 1, 0);
		}
	}

	/* enable generate High Speed clock, non continue */
	if (pinfo->mipi.non_continue_en) {
		set_reg(mipi_dsi_base + MIPIDSI_LPCLK_CTRL_OFFSET, 0x3, 2, 0);
	} else {
		set_reg(mipi_dsi_base + MIPIDSI_LPCLK_CTRL_OFFSET, 0x1, 2, 0);
	}

	if ((pinfo->mipi.dsi_version == DSI_1_2_VERSION)
		&& is_ifbc_vesa_panel(hisifd)) {
		set_reg(mipi_dsi_base + MIPIDSI_DSC_PARAMETER_OFFSET, 0x01, 32, 0);
	}


	pctrl_dphytx_stopcnt_config(hisifd);
	return 0;
}

static int mipi_dsi_off_sub(struct hisi_fb_data_type *hisifd, char __iomem *mipi_dsi_base)
{
	if (NULL == hisifd) {
		HISI_FB_ERR("hisifd is NULL");
		return -EINVAL;
	}
	if(NULL == mipi_dsi_base){
		HISI_FB_ERR("mipi_dsi_base is NULL");
		return -EINVAL;
	}

	/* switch to cmd mode */
	set_reg(mipi_dsi_base + MIPIDSI_MODE_CFG_OFFSET, 0x1, 1, 0);
	/* cmd mode: low power mode */
	set_reg(mipi_dsi_base + MIPIDSI_CMD_MODE_CFG_OFFSET, 0x7f, 7, 8);
	set_reg(mipi_dsi_base + MIPIDSI_CMD_MODE_CFG_OFFSET, 0xf, 4, 16);
	set_reg(mipi_dsi_base + MIPIDSI_CMD_MODE_CFG_OFFSET, 0x1, 1, 24);

	/* disable generate High Speed clock */
	set_reg(mipi_dsi_base + MIPIDSI_LPCLK_CTRL_OFFSET, 0x0, 1, 0);

	/* shutdown d_phy */
	set_reg(mipi_dsi_base +  MIPIDSI_PHY_RSTZ_OFFSET, 0x0, 3, 0);

	return 0;
}

//lint -save -e778 -e774 -e747 -e732
static int mipi_dsi_ulps_enter(struct hisi_fb_data_type *hisifd,
	char __iomem *mipi_dsi_base)
{
	uint32_t tmp;
	uint32_t cmp_ulpsactivenot_val = 0;
	uint32_t cmp_stopstate_val = 0;
	uint32_t try_times;

	if (NULL == hisifd || NULL == mipi_dsi_base) {
		HISI_FB_ERR("hisifd or mipi_dsi_base is NULL.\n");
		return 0;
	}

	HISI_FB_DEBUG("fb%d, +!\n", hisifd->index);

	if (hisifd->panel_info.mipi.lane_nums >= DSI_4_LANES) {
		cmp_ulpsactivenot_val = (BIT(5) | BIT(8) | BIT(10) | BIT(12));
		cmp_stopstate_val = (BIT(4) | BIT(7) | BIT(9) | BIT(11));
	} else if (hisifd->panel_info.mipi.lane_nums >= DSI_3_LANES) {
		cmp_ulpsactivenot_val = (BIT(5) | BIT(8) | BIT(10));
		cmp_stopstate_val = (BIT(4) | BIT(7) | BIT(9));
	} else if (hisifd->panel_info.mipi.lane_nums >= DSI_2_LANES) {
		cmp_ulpsactivenot_val = (BIT(5) | BIT(8));
		cmp_stopstate_val = (BIT(4) | BIT(7));
	} else {
		cmp_ulpsactivenot_val = (BIT(5));
		cmp_stopstate_val = (BIT(4));
	}

	tmp = inp32(mipi_dsi_base + MIPIDSI_LPCLK_CTRL_OFFSET) & BIT(1);
	if (tmp && (hisifd->panel_info.mipi.phy_mode == DPHY_MODE)) {
		cmp_stopstate_val |= (BIT(2));
	}

	// check DPHY data and clock lane stopstate
	try_times = 0;
	tmp = inp32(mipi_dsi_base + MIPIDSI_PHY_STATUS_OFFSET);
	while ((tmp & cmp_stopstate_val) != cmp_stopstate_val) {
		udelay(10);
		if (++try_times > 100) {
			HISI_FB_ERR("fb%d, check phy data and clk lane stop state failed! PHY_STATUS=0x%x.\n",
				hisifd->index, tmp);
			return 0;
		}

		tmp = inp32(mipi_dsi_base + MIPIDSI_PHY_STATUS_OFFSET);
	}


	// disable DPHY clock lane's Hight Speed Clock
	set_reg(mipi_dsi_base + MIPIDSI_LPCLK_CTRL_OFFSET, 0x0, 1, 0);

	// request that data lane enter ULPS
	set_reg(mipi_dsi_base + MIPIDSI_PHY_ULPS_CTRL_OFFSET, 0x4, 4, 0);

	//check DPHY data lane ulpsactivenot_status
	try_times = 0;
	tmp = inp32(mipi_dsi_base + MIPIDSI_PHY_STATUS_OFFSET);
	while ((tmp & cmp_ulpsactivenot_val) != 0) {
		udelay(10);
		if (++try_times > 100) {
			HISI_FB_ERR("fb%d, request phy data lane enter ulps failed! PHY_STATUS=0x%x.\n",
				hisifd->index, tmp);
			break;
		}

		tmp = inp32(mipi_dsi_base + MIPIDSI_PHY_STATUS_OFFSET);
	}

	// request that clock lane enter ULPS
	if (hisifd->panel_info.mipi.phy_mode == DPHY_MODE) {
		set_reg(mipi_dsi_base + MIPIDSI_PHY_ULPS_CTRL_OFFSET, 0x5, 4, 0);

		// check DPHY clock lane ulpsactivenot_status
		try_times = 0;
		tmp = inp32(mipi_dsi_base + MIPIDSI_PHY_STATUS_OFFSET);
		while ((tmp & BIT(3)) != 0) {
			udelay(10);
			if (++try_times > 100) {
				HISI_FB_ERR("fb%d, request phy clk lane enter ulps failed! PHY_STATUS=0x%x.\n",
					hisifd->index, tmp);
				break;
			}

			tmp = inp32(mipi_dsi_base + MIPIDSI_PHY_STATUS_OFFSET);
		}
	}

	//bit13 lock sel enable (dual_mipi_panel bit29 set 1) ,colse clock gate
	set_reg(hisifd->pctrl_base + PERI_CTRL30, 0x1, 1, 13);
	if (is_dual_mipi_panel(hisifd)) {
		set_reg(hisifd->pctrl_base + PERI_CTRL30, 0x1, 1, 29);
	}


	HISI_FB_DEBUG("fb%d, -!\n", hisifd->index);

	return 0;
}

static int mipi_dsi_ulps_exit(struct hisi_fb_data_type *hisifd,
	char __iomem *mipi_dsi_base)
{
	uint32_t tmp = 0;
	uint32_t cmp_ulpsactivenot_val = 0;
	uint32_t cmp_stopstate_val = 0;
	uint32_t try_times = 0;
	uint32_t need_pll_retry = 0;

	if (NULL == hisifd) {
		HISI_FB_ERR("hisifd is NULL");
		return -EINVAL;
	}
	if(NULL == mipi_dsi_base){
		HISI_FB_ERR("mipi_dsi_base is NULL");
		return -EINVAL;
	}

	HISI_FB_DEBUG("fb%d, +!\n", hisifd->index);

	if (hisifd->panel_info.mipi.lane_nums >= DSI_4_LANES) {
		cmp_ulpsactivenot_val = (BIT(3) | BIT(5) | BIT(8) | BIT(10) | BIT(12));
		cmp_stopstate_val = (BIT(2) | BIT(4) | BIT(7) | BIT(9) | BIT(11));
	} else if (hisifd->panel_info.mipi.lane_nums >= DSI_3_LANES) {
		cmp_ulpsactivenot_val = (BIT(3) | BIT(5) | BIT(8) | BIT(10));
		cmp_stopstate_val = (BIT(2) | BIT(4) | BIT(7) | BIT(9));
	} else if (hisifd->panel_info.mipi.lane_nums >= DSI_2_LANES) {
		cmp_ulpsactivenot_val = (BIT(3) | BIT(5) | BIT(8));
		cmp_stopstate_val = (BIT(2) | BIT(4) | BIT(7));
	} else {
		cmp_ulpsactivenot_val = (BIT(3) | BIT(5));
		cmp_stopstate_val = (BIT(2) | BIT(4));
	}
	if (hisifd->panel_info.mipi.phy_mode == CPHY_MODE) {
		cmp_ulpsactivenot_val &= (~ BIT(3));
		cmp_stopstate_val &= (~ BIT(2));
	}


	//wait pll clk
	udelay(100);
	//bit13 lock sel enable (dual_mipi_panel bit29 set 0) ,open clock gate
	set_reg(hisifd->pctrl_base + PERI_CTRL30, 0x0, 1, 13);
	if (is_dual_mipi_panel(hisifd)) {
		set_reg(hisifd->pctrl_base + PERI_CTRL30, 0x0, 1, 29);
	}

	// enable DPHY PLL, force_pll = 1
	//outp32(mipi_dsi_base + MIPIDSI_PHY_RSTZ_OFFSET, 0xF); ???

	// request that data lane and clock lane exit ULPS
	outp32(mipi_dsi_base + MIPIDSI_PHY_ULPS_CTRL_OFFSET, 0xF);
	try_times= 0;
	tmp = inp32(mipi_dsi_base + MIPIDSI_PHY_STATUS_OFFSET);
	while ((tmp & cmp_ulpsactivenot_val) != cmp_ulpsactivenot_val) {
		udelay(10);
		if (++try_times > 100) {
			HISI_FB_ERR("fb%d, request data clock lane exit ulps fail!PHY_STATUS=0x%x.\n",
				hisifd->index, tmp);
			need_pll_retry = BIT(0);
			break;
		}

		tmp = inp32(mipi_dsi_base + MIPIDSI_PHY_STATUS_OFFSET);
	}

	// mipi spec
	mdelay(1);

	// clear PHY_ULPS_CTRL
	outp32(mipi_dsi_base + MIPIDSI_PHY_ULPS_CTRL_OFFSET, 0x0);


//check DPHY data lane cmp_stopstate_val
	try_times = 0;
	tmp = inp32(mipi_dsi_base + MIPIDSI_PHY_STATUS_OFFSET);
	while ((tmp & cmp_stopstate_val) != cmp_stopstate_val) {
		udelay(10);
		if (++try_times > 100) {
			HISI_FB_ERR("fb%d, check phy data clk lane stop state failed! PHY_STATUS=0x%x.\n",
				hisifd->index, tmp);
			need_pll_retry |= BIT(1);
			break;
		}

		tmp = inp32(mipi_dsi_base + MIPIDSI_PHY_STATUS_OFFSET);
	}

	// enable DPHY clock lane's Hight Speed Clock
	set_reg(mipi_dsi_base + MIPIDSI_LPCLK_CTRL_OFFSET, 0x1, 1, 0);

	//reset dsi
	outp32(mipi_dsi_base + MIPIDSI_PWR_UP_OFFSET, 0x0);
	udelay(5);
	// Power_up dsi
	outp32(mipi_dsi_base + MIPIDSI_PWR_UP_OFFSET, 0x1);
	HISI_FB_DEBUG("fb%d, -!\n", hisifd->index);

	return 0;
}
//lint -restore

int mipi_dsi_ulps_cfg(struct hisi_fb_data_type *hisifd, int enable)
{
	int ret = 0;

	if (NULL == hisifd) {
		HISI_FB_ERR("hisifd is NULL");
		return -EINVAL;
	}

	HISI_FB_DEBUG("fb%d, +.\n", hisifd->index);

	if (enable) {
		mipi_dsi_ulps_exit(hisifd, hisifd->mipi_dsi0_base);
		if (is_dual_mipi_panel(hisifd))
			mipi_dsi_ulps_exit(hisifd, hisifd->mipi_dsi1_base);
	} else {
		mipi_dsi_ulps_enter(hisifd, hisifd->mipi_dsi0_base);
		if (is_dual_mipi_panel(hisifd))
			mipi_dsi_ulps_enter(hisifd, hisifd->mipi_dsi1_base);
	}

	HISI_FB_DEBUG("fb%d, -.\n", hisifd->index);

	return ret;
}

void mipi_dsi_reset(struct hisi_fb_data_type *hisifd)
{
	if (NULL == hisifd) {
		HISI_FB_ERR("hisifd is NULL");
		return;
	}
	set_reg(hisifd->mipi_dsi0_base + MIPIDSI_PWR_UP_OFFSET, 0x0, 1, 0);
	msleep(2);
	set_reg(hisifd->mipi_dsi0_base + MIPIDSI_PWR_UP_OFFSET, 0x1, 1, 0);
}

/*******************************************************************************
** MIPI DPHY GPIO for FPGA
*/
#define GPIO_MIPI_DPHY_PG_SEL_A_NAME	"pg_sel_a"
#define GPIO_MIPI_DPHY_PG_SEL_B_NAME	"pg_sel_b"
#define GPIO_MIPI_DPHY_TX_RX_A_NAME	"tx_rx_a"
#define GPIO_MIPI_DPHY_TX_RX_B_NAME	"tx_rx_b"

static uint32_t gpio_pg_sel_a = GPIO_PG_SEL_A;
static uint32_t gpio_tx_rx_a = GPIO_TX_RX_A;
static uint32_t gpio_pg_sel_b = GPIO_PG_SEL_B;
static uint32_t gpio_tx_rx_b = GPIO_TX_RX_B;

static struct gpio_desc mipi_dphy_gpio_request_cmds[] = {
	{DTYPE_GPIO_REQUEST, WAIT_TYPE_MS, 0,
		GPIO_MIPI_DPHY_PG_SEL_A_NAME, &gpio_pg_sel_a, 0},
	{DTYPE_GPIO_REQUEST, WAIT_TYPE_MS, 0,
		GPIO_MIPI_DPHY_PG_SEL_B_NAME, &gpio_pg_sel_b, 0},
	{DTYPE_GPIO_REQUEST, WAIT_TYPE_MS, 0,
		GPIO_MIPI_DPHY_TX_RX_A_NAME, &gpio_tx_rx_a, 0},
	{DTYPE_GPIO_REQUEST, WAIT_TYPE_MS, 0,
		GPIO_MIPI_DPHY_TX_RX_B_NAME, &gpio_tx_rx_b, 0},
};

static struct gpio_desc mipi_dphy_gpio_free_cmds[] = {
	{DTYPE_GPIO_FREE, WAIT_TYPE_MS, 0,
		GPIO_MIPI_DPHY_PG_SEL_A_NAME, &gpio_pg_sel_a, 0},
	{DTYPE_GPIO_FREE, WAIT_TYPE_MS, 0,
		GPIO_MIPI_DPHY_PG_SEL_B_NAME, &gpio_pg_sel_b, 0},
	{DTYPE_GPIO_FREE, WAIT_TYPE_MS, 0,
		GPIO_MIPI_DPHY_TX_RX_A_NAME, &gpio_tx_rx_a, 0},
	{DTYPE_GPIO_FREE, WAIT_TYPE_MS, 0,
		GPIO_MIPI_DPHY_TX_RX_B_NAME, &gpio_tx_rx_b, 0},

};

static struct gpio_desc mipi_dphy_gpio_normal_cmds[] = {
	{DTYPE_GPIO_OUTPUT, WAIT_TYPE_MS, 0,
		GPIO_MIPI_DPHY_PG_SEL_A_NAME, &gpio_pg_sel_a, 0},
	{DTYPE_GPIO_OUTPUT, WAIT_TYPE_MS, 0,
		GPIO_MIPI_DPHY_TX_RX_A_NAME, &gpio_tx_rx_a, 1},

	{DTYPE_GPIO_OUTPUT, WAIT_TYPE_MS, 0,
		GPIO_MIPI_DPHY_PG_SEL_B_NAME, &gpio_pg_sel_b, 0},
	{DTYPE_GPIO_OUTPUT, WAIT_TYPE_MS, 0,
		GPIO_MIPI_DPHY_TX_RX_B_NAME, &gpio_tx_rx_b, 1},
};

static struct gpio_desc mipi_dphy_gpio_lowpower_cmds[] = {
	{DTYPE_GPIO_OUTPUT, WAIT_TYPE_MS, 0,
		GPIO_MIPI_DPHY_PG_SEL_A_NAME, &gpio_pg_sel_a, 0},
	{DTYPE_GPIO_OUTPUT, WAIT_TYPE_MS, 0,
		GPIO_MIPI_DPHY_TX_RX_A_NAME, &gpio_tx_rx_a, 0},

	{DTYPE_GPIO_OUTPUT, WAIT_TYPE_MS, 0,
		GPIO_MIPI_DPHY_PG_SEL_B_NAME, &gpio_pg_sel_b, 0},
	{DTYPE_GPIO_OUTPUT, WAIT_TYPE_MS, 0,
		GPIO_MIPI_DPHY_TX_RX_B_NAME, &gpio_tx_rx_b, 0},
};

static int mipi_dsi_dphy_fastboot_fpga(struct hisi_fb_data_type *hisifd)
{
	if (NULL == hisifd) {
		HISI_FB_ERR("hisifd is NULL");
		return -EINVAL;
	}

	if (g_fpga_flag == 1) {
		/* mpi dphy gpio request */
		gpio_cmds_tx(mipi_dphy_gpio_request_cmds,
			ARRAY_SIZE(mipi_dphy_gpio_request_cmds));
	}

	return 0;
}

static int mipi_dsi_dphy_on_fpga(struct hisi_fb_data_type *hisifd)
{
	if (NULL == hisifd) {
		HISI_FB_ERR("hisifd is NULL");
		return -EINVAL;
	}

	if (hisifd->index == EXTERNAL_PANEL_IDX)
		return 0;

	if (g_fpga_flag == 1) {
		/* mipi dphy gpio request */
		gpio_cmds_tx(mipi_dphy_gpio_request_cmds,
			ARRAY_SIZE(mipi_dphy_gpio_request_cmds));

		/* mipi dphy gpio normal */
		gpio_cmds_tx(mipi_dphy_gpio_normal_cmds, \
			ARRAY_SIZE(mipi_dphy_gpio_normal_cmds));
	}

	return 0;
}

static int mipi_dsi_dphy_off_fpga(struct hisi_fb_data_type *hisifd)
{
	if (NULL == hisifd) {
		HISI_FB_ERR("hisifd is NULL");
		return -EINVAL;
	}

	if (hisifd->index == EXTERNAL_PANEL_IDX)
		return 0;

	if (g_fpga_flag == 1) {
		/* mipi dphy gpio lowpower */
		gpio_cmds_tx(mipi_dphy_gpio_lowpower_cmds, \
			ARRAY_SIZE(mipi_dphy_gpio_lowpower_cmds));
		/* mipi dphy gpio free */
		gpio_cmds_tx(mipi_dphy_gpio_free_cmds, \
			ARRAY_SIZE(mipi_dphy_gpio_free_cmds));
	}

	return 0;
}


/*******************************************************************************
**
*/
int mipi_dsi_set_fastboot(struct platform_device *pdev)
{
	int ret = 0;
	struct hisi_fb_data_type *hisifd = NULL;
	struct hisi_panel_info *pinfo = NULL;

	if (NULL == pdev) {
		HISI_FB_ERR("pdev is NULL");
		return -EINVAL;
	}
	hisifd = platform_get_drvdata(pdev);
	if (NULL == hisifd) {
		HISI_FB_ERR("hisifd is NULL");
		return -EINVAL;
	}

	HISI_FB_DEBUG("fb%d, +.\n", hisifd->index);

	mipi_dsi_dphy_fastboot_fpga(hisifd);

	mipi_dsi_clk_enable(hisifd);

	pinfo = &(hisifd->panel_info);//lint !e838
	if (pinfo) {
		memset(&(pinfo->dsi_phy_ctrl), 0, sizeof(struct mipi_dsi_phy_ctrl));
		if ((g_dss_version_tag & FB_ACCEL_DSSV320) && (g_fpga_flag == 0)) {
			//get_dsi_phy_ctrl_asic(hisifd, &(pinfo->dsi_phy_ctrl));
			get_dsi_phy_ctrl_v320(hisifd, &(pinfo->dsi_phy_ctrl), pinfo->mipi.dsi_bit_clk_upt);
		} else {
			get_dsi_phy_ctrl(hisifd, &(pinfo->dsi_phy_ctrl));
		}
	}

	ret = panel_next_set_fastboot(pdev);

	HISI_FB_DEBUG("fb%d, -.\n", hisifd->index);

	return ret;
}

int mipi_dsi_on(struct platform_device *pdev)
{
	struct hisi_fb_data_type *hisifd = NULL;

	if (NULL == pdev) {
		HISI_FB_ERR("pdev is NULL");
		return -EINVAL;
	}
	hisifd = platform_get_drvdata(pdev);
	if (NULL == hisifd) {
		HISI_FB_ERR("hisifd is NULL");
		return -EINVAL;
	}

	HISI_FB_DEBUG("fb%d, +.\n", hisifd->index);

	mipi_dsi_dphy_on_fpga(hisifd);

	/* set LCD init step before LCD on*/
	hisifd->panel_info.lcd_init_step = LCD_INIT_POWER_ON;
	panel_next_on(pdev);

	//dis-reset
	//ip_reset_dis_dsi0, ip_reset_dis_dsi1
	if (hisifd->index == PRIMARY_PANEL_IDX) {
		if (is_dual_mipi_panel(hisifd))
			outp32(hisifd->peri_crg_base + PERRSTDIS3, 0x30000000);
		else
			outp32(hisifd->peri_crg_base + PERRSTDIS3, 0x10000000);
	} else if (hisifd->index == EXTERNAL_PANEL_IDX) {
		outp32(hisifd->peri_crg_base + PERRSTDIS3, 0x20000000);
	} else {
		HISI_FB_ERR("fb%d, not supported!\n", hisifd->index);
	}

	mipi_dsi_clk_enable(hisifd);

	if (hisifd->index == PRIMARY_PANEL_IDX) {
		mipi_dsi_on_sub1(hisifd, hisifd->mipi_dsi0_base);
		if (is_dual_mipi_panel(hisifd))
			mipi_dsi_on_sub1(hisifd, hisifd->mipi_dsi1_base);

		mipi_dsi_pll_status_check_ec(hisifd, hisifd->mipi_dsi0_base);
		if (is_dual_mipi_panel(hisifd)) {
			mipi_dsi_pll_status_check_ec(hisifd, hisifd->mipi_dsi1_base);
		}
	} else if (hisifd->index == EXTERNAL_PANEL_IDX) {
		mipi_dsi_on_sub1(hisifd, hisifd->mipi_dsi1_base);
		mipi_dsi_pll_status_check_ec(hisifd, hisifd->mipi_dsi1_base);
	} else {
		HISI_FB_ERR("fb%d, not supported!\n", hisifd->index);
	}
	panel_next_on(pdev);

	if (hisifd->index == PRIMARY_PANEL_IDX) {
		mipi_dsi_on_sub2(hisifd, hisifd->mipi_dsi0_base);
		if (is_dual_mipi_panel(hisifd))
			mipi_dsi_on_sub2(hisifd, hisifd->mipi_dsi1_base);
	} else if (hisifd->index == EXTERNAL_PANEL_IDX) {
		mipi_dsi_on_sub2(hisifd, hisifd->mipi_dsi1_base);
	} else {
		HISI_FB_ERR("fb%d, not supported!\n", hisifd->index);
	}

	/* mipi hs video/command mode */
	panel_next_on(pdev);

	HISI_FB_DEBUG("fb%d, -.\n", hisifd->index);

	return 0;
}

int mipi_dsi_off(struct platform_device *pdev)
{
	int ret = 0;
	struct hisi_fb_data_type *hisifd = NULL;

	if (NULL == pdev) {
		HISI_FB_ERR("pdev is NULL");
		return -EINVAL;
	}
	hisifd = platform_get_drvdata(pdev);
	if (NULL == hisifd) {
		HISI_FB_ERR("hisifd is NULL");
		return -EINVAL;
	}

	HISI_FB_DEBUG("fb%d, +.\n", hisifd->index);

	/* set LCD uninit step before LCD off*/
	hisifd->panel_info.lcd_uninit_step = LCD_UNINIT_MIPI_HS_SEND_SEQUENCE;
	ret = panel_next_off(pdev);

	if (hisifd->panel_info.lcd_uninit_step_support) {
		/* TODO: add MIPI LP mode here if necessary */
		/* MIPI LP mode end */
		ret = panel_next_off(pdev);
	}

	if (hisifd->index == PRIMARY_PANEL_IDX) {
		mipi_dsi_off_sub(hisifd, hisifd->mipi_dsi0_base);
		if (is_dual_mipi_panel(hisifd))
			mipi_dsi_off_sub(hisifd, hisifd->mipi_dsi1_base);
	} else if (hisifd->index == EXTERNAL_PANEL_IDX) {
		mipi_dsi_off_sub(hisifd, hisifd->mipi_dsi1_base);
	} else {
		HISI_FB_ERR("fb%d, not supported!\n", hisifd->index);
	}

	mipi_dsi_clk_disable(hisifd);

	mipi_dsi_dphy_off_fpga(hisifd);

	// reset DSI
	if (hisifd->index == PRIMARY_PANEL_IDX) {
		if (is_dual_mipi_panel(hisifd))
			outp32(hisifd->peri_crg_base + PERRSTEN3, 0x30000000);
		else
			outp32(hisifd->peri_crg_base + PERRSTEN3, 0x10000000);
	} else if (hisifd->index == EXTERNAL_PANEL_IDX) {
		outp32(hisifd->peri_crg_base + PERRSTEN3, 0x20000000);
	} else {
		HISI_FB_ERR("fb%d, not supported!\n", hisifd->index);
	}

	if (hisifd->panel_info.lcd_uninit_step_support) {
		ret = panel_next_off(pdev);
	}

	HISI_FB_DEBUG("fb%d, -.\n", hisifd->index);

	return ret;
}

/*lint -e712 -e838 */
// TODO: Only for dallas video mode
static void mipi_dsi_bit_clk_upt_set_video(struct hisi_fb_data_type *hisifd,
	char __iomem *mipi_dsi_base, struct mipi_dsi_phy_ctrl* phy_ctrl)
{
	struct hisi_panel_info *pinfo = NULL;
	uint32_t hline_time = 0;
	uint32_t hsa_time = 0;
	uint32_t hbp_time = 0;
	uint32_t pixel_clk = 0;
	dss_rect_t rect;
	unsigned long dw_jiffies = 0;
	uint32_t tmp;
	bool is_ready = false;
	uint32_t i;

	if (hisifd == NULL) {
		return;
	}

	if (phy_ctrl == NULL) {
		return;
	}

	pinfo = &(hisifd->panel_info);

	rect.x = 0;
	rect.y = 0;
	rect.w = pinfo->xres;
	rect.h = pinfo->yres;

	mipi_ifbc_get_rect(hisifd, &rect);

	/*************************Configure the DPHY start*************************/
	// physical configuration I, Q
	mipi_config_phy_test_code(mipi_dsi_base, 0x00010010, phy_ctrl->rg_hstx_ckg_sel << 1);

	// physical configuration PLL I
	mipi_config_phy_test_code(mipi_dsi_base, 0x00010014, (phy_ctrl->rg_pll_fbd_s << 4) +
		(phy_ctrl->rg_pll_enswc << 3) + (phy_ctrl->rg_pll_enbwt << 2) + phy_ctrl->rg_pll_chp);

	// physical configuration PLL II, M
	mipi_config_phy_test_code(mipi_dsi_base, 0x00010015, phy_ctrl->rg_pll_fbd_p);
	mipi_config_phy_test_code(mipi_dsi_base, 0x00010016, (phy_ctrl->rg_pll_cp << 5) +
		(phy_ctrl->rg_pll_lpf_cs << 4) + phy_ctrl->rg_pll_refsel);
	// physical configuration PLL IV, N
	mipi_config_phy_test_code(mipi_dsi_base, 0x00010017, phy_ctrl->rg_pll_pre_p);

	// sets the analog characteristic of V reference in D-PHY TX
	mipi_config_phy_test_code(mipi_dsi_base, 0x0001001D, phy_ctrl->rg_vrefsel_vcm);
	//MISC AFE Configuration
	mipi_config_phy_test_code(mipi_dsi_base, 0x0001001E, (phy_ctrl->rg_pll_cp_p << 5) +
		(phy_ctrl->reload_sel << 4) + (phy_ctrl->rg_phase_gen_en << 3) +
		(phy_ctrl->rg_band_sel << 2) + (phy_ctrl->pll_power_down << 1) +
		phy_ctrl->pll_register_override);

	//reload_command
	mipi_config_phy_test_code(mipi_dsi_base, 0x0001001F, phy_ctrl->load_command);

	//pre_delay of clock lane request setting
	mipi_config_phy_test_code(mipi_dsi_base, MIPIDSI_PHY_TST_CLK_PRE_DELAY, DSS_REDUCE(phy_ctrl->clk_pre_delay));

	//post_delay of clock lane request setting
	mipi_config_phy_test_code(mipi_dsi_base, MIPIDSI_PHY_TST_CLK_POST_DELAY, DSS_REDUCE(phy_ctrl->clk_post_delay));

	//clock lane timing ctrl - t_lpx
	mipi_config_phy_test_code(mipi_dsi_base, MIPIDSI_PHY_TST_CLK_TLPX, DSS_REDUCE(phy_ctrl->clk_t_lpx));

	//clock lane timing ctrl - t_hs_prepare
	mipi_config_phy_test_code(mipi_dsi_base, MIPIDSI_PHY_TST_CLK_PREPARE, DSS_REDUCE(phy_ctrl->clk_t_hs_prepare));

	//clock lane timing ctrl - t_hs_zero
	mipi_config_phy_test_code(mipi_dsi_base, MIPIDSI_PHY_TST_CLK_ZERO, DSS_REDUCE(phy_ctrl->clk_t_hs_zero));

	//clock lane timing ctrl - t_hs_trial
	mipi_config_phy_test_code(mipi_dsi_base, MIPIDSI_PHY_TST_CLK_TRAIL, phy_ctrl->clk_t_hs_trial);

	for (i = 0; i <= pinfo->mipi.lane_nums; i++) {
		//data lane pre_delay
		tmp = MIPIDSI_PHY_TST_DATA_PRE_DELAY + (i << 4);
		mipi_config_phy_test_code(mipi_dsi_base, tmp, DSS_REDUCE(phy_ctrl->data_pre_delay));

		//data lane post_delay
		tmp = MIPIDSI_PHY_TST_DATA_POST_DELAY + (i << 4);
		mipi_config_phy_test_code(mipi_dsi_base, tmp, DSS_REDUCE(phy_ctrl->data_post_delay));

		//data lane timing ctrl - t_lpx
		tmp = MIPIDSI_PHY_TST_DATA_TLPX + (i << 4);
		mipi_config_phy_test_code(mipi_dsi_base, tmp, DSS_REDUCE(phy_ctrl->data_t_lpx));

		//data lane timing ctrl - t_hs_prepare
		tmp = MIPIDSI_PHY_TST_DATA_PREPARE + (i << 4);
		mipi_config_phy_test_code(mipi_dsi_base, tmp, DSS_REDUCE(phy_ctrl->data_t_hs_prepare));

		//data lane timing ctrl - t_hs_zero
		tmp = MIPIDSI_PHY_TST_DATA_ZERO + (i << 4);
		mipi_config_phy_test_code(mipi_dsi_base, tmp, DSS_REDUCE(phy_ctrl->data_t_hs_zero));

		//data lane timing ctrl - t_hs_trial
		tmp = MIPIDSI_PHY_TST_DATA_TRAIL + (i << 4);
		mipi_config_phy_test_code(mipi_dsi_base, tmp, phy_ctrl->data_t_hs_trial);

		//data lane timing ctrl - t_ta_go
		tmp = MIPIDSI_PHY_TST_DATA_TA_GO + (i << 4);
		mipi_config_phy_test_code(mipi_dsi_base, tmp, DSS_REDUCE(phy_ctrl->data_t_ta_go));

		//data lane timing ctrl - t_ta_get
		tmp = MIPIDSI_PHY_TST_DATA_TA_GET + (i << 4);
		mipi_config_phy_test_code(mipi_dsi_base, tmp, DSS_REDUCE(phy_ctrl->data_t_ta_get));
	}

	is_ready = false;
	dw_jiffies = jiffies + HZ / 2;
	do {
		tmp = inp32(mipi_dsi_base + MIPIDSI_PHY_STATUS_OFFSET);
		if ((tmp & 0x00000001) == 0x00000001) {
			is_ready = true;
			break;
		}
	} while (time_after(dw_jiffies, jiffies));

	if (!is_ready) {
		HISI_FB_ERR("fb%d, phylock is not ready!MIPIDSI_PHY_STATUS_OFFSET=0x%x.\n",
			hisifd->index, tmp);
	}

	/*************************Configure the DPHY end*************************/

	// phy_stop_wait_time
	set_reg(mipi_dsi_base + MIPIDSI_PHY_IF_CFG_OFFSET, phy_ctrl->phy_stop_wait_time, 8, 8);

	/*
	** 4. Define the DPI Horizontal timing configuration:
	**
	** Hsa_time = HSA*(PCLK period/Clk Lane Byte Period);
	** Hbp_time = HBP*(PCLK period/Clk Lane Byte Period);
	** Hline_time = (HSA+HBP+HACT+HFP)*(PCLK period/Clk Lane Byte Period);
	*/
	pixel_clk = mipi_pixel_clk(hisifd);
	hsa_time = pinfo->ldi.h_pulse_width * phy_ctrl->lane_byte_clk / pixel_clk;
	hbp_time = pinfo->ldi.h_back_porch * phy_ctrl->lane_byte_clk / pixel_clk;
	hline_time = (pinfo->ldi.h_pulse_width + pinfo->ldi.h_back_porch +
		rect.w + pinfo->ldi.h_front_porch) * phy_ctrl->lane_byte_clk / pixel_clk;

	set_reg(mipi_dsi_base + MIPIDSI_VID_HSA_TIME_OFFSET, hsa_time, 12, 0);
	set_reg(mipi_dsi_base + MIPIDSI_VID_HBP_TIME_OFFSET, hbp_time, 12, 0);
	set_reg(mipi_dsi_base + MIPIDSI_VID_HLINE_TIME_OFFSET, hline_time, 15, 0);

	// Configure core's phy parameters
	set_reg(mipi_dsi_base + MIPIDSI_PHY_TMR_LPCLK_CFG_OFFSET, phy_ctrl->clk_lane_lp2hs_time, 10, 0);
	set_reg(mipi_dsi_base + MIPIDSI_PHY_TMR_LPCLK_CFG_OFFSET, phy_ctrl->clk_lane_hs2lp_time, 10, 16);

	set_reg(mipi_dsi_base + MIPIDSI_PHY_TMR_CFG_OFFSET, phy_ctrl->data_lane_lp2hs_time, 10, 0);
	set_reg(mipi_dsi_base + MIPIDSI_PHY_TMR_CFG_OFFSET, phy_ctrl->data_lane_hs2lp_time, 10, 16);
}
/*lint +e712 +e838 */

static void mipi_dsi_bit_clk_upt_set_cmd(struct hisi_fb_data_type *hisifd,
	char __iomem *mipi_dsi_base, struct mipi_dsi_phy_ctrl* phy_ctrl)
{
	struct hisi_panel_info *pinfo;

	if (hisifd == NULL) {
		return;
	}

	if (phy_ctrl == NULL) {
		return;
	}

	pinfo = &(hisifd->panel_info);

	// config PLL M N Q
	if (phy_ctrl->rg_pll_pre_p > pinfo->dsi_phy_ctrl.rg_pll_pre_p) {
		// physical configuration PLL IV, N
		mipi_config_phy_test_code(mipi_dsi_base, 0x00010017, phy_ctrl->rg_pll_pre_p);

		// physical configuration PLL II, M
		mipi_config_phy_test_code(mipi_dsi_base, 0x00010015, phy_ctrl->rg_pll_fbd_p);
	} else {
		// physical configuration PLL II, M
		mipi_config_phy_test_code(mipi_dsi_base, 0x00010015, phy_ctrl->rg_pll_fbd_p);

		// physical configuration PLL IV, N
		mipi_config_phy_test_code(mipi_dsi_base, 0x00010017, phy_ctrl->rg_pll_pre_p);
	}

	// physical configuration PLL I
	mipi_config_phy_test_code(mipi_dsi_base, 0x00010014, (phy_ctrl->rg_pll_fbd_s << 4) +
		(phy_ctrl->rg_pll_enswc << 3) + (phy_ctrl->rg_pll_enbwt << 2) + phy_ctrl->rg_pll_chp);

	//reload_command
	mipi_config_phy_test_code(mipi_dsi_base, 0x0001001F, pinfo->dsi_phy_ctrl.load_command);
	// Configure core's phy parameters
	set_reg(mipi_dsi_base + MIPIDSI_PHY_TMR_LPCLK_CFG_OFFSET,
			pinfo->dsi_phy_ctrl.clk_lane_lp2hs_time, 10, 0);
	set_reg(mipi_dsi_base + MIPIDSI_PHY_TMR_LPCLK_CFG_OFFSET,
			pinfo->dsi_phy_ctrl.clk_lane_hs2lp_time, 10, 16);

	set_reg(mipi_dsi_base + MIPIDSI_PHY_TMR_RD_CFG_OFFSET, 0x7FFF, 15, 0);
	set_reg(mipi_dsi_base + MIPIDSI_PHY_TMR_CFG_OFFSET,
			pinfo->dsi_phy_ctrl.data_lane_lp2hs_time, 10, 0);
	set_reg(mipi_dsi_base + MIPIDSI_PHY_TMR_CFG_OFFSET,
			pinfo->dsi_phy_ctrl.data_lane_hs2lp_time, 10, 16);

	// escape clock dividor
	outp32(mipi_dsi_base + MIPIDSI_CLKMGR_CFG_OFFSET,
		(phy_ctrl->clk_division + (phy_ctrl->clk_division << 8)));
}

static bool check_pctrl_trstop_flag(struct hisi_fb_data_type *hisifd)
{
	bool is_ready = false;
	int count;
	uint32_t tmp = 0;
	int time_count = 40;

	if (is_dual_mipi_panel(hisifd)) {
		for(count = 0; count < time_count; count++) {
			tmp = inp32(hisifd->pctrl_base + PERI_STAT0);
			if ((tmp & 0xC0000000) == 0xC0000000) {
				is_ready = true;
				break;
			}
			udelay(2);
		}
	} else {
		for(count = 0; count < time_count; count++) {
			tmp = inp32(hisifd->pctrl_base + PERI_STAT0);
			if ((tmp & 0x80000000) == 0x80000000) {
				is_ready = true;
				break;
			}
			udelay(2);
		}
	}

	return is_ready;
}

int mipi_dsi_bit_clk_upt_isr_handler(struct hisi_fb_data_type *hisifd)
{
	struct mipi_dsi_phy_ctrl phy_ctrl = {0};
	struct hisi_panel_info *pinfo;
	uint32_t dsi_bit_clk_upt;
	bool is_ready;
	uint8_t esd_enable;

	if (NULL == hisifd) {
		HISI_FB_ERR("hisifd is null!\n");
		return 0;
	}
	pinfo = &(hisifd->panel_info);
	dsi_bit_clk_upt = pinfo->mipi.dsi_bit_clk_upt;

	if (hisifd->index != PRIMARY_PANEL_IDX) {
		HISI_FB_ERR("fb%d, not support!\n", hisifd->index);
		return 0;
	}

	if (dsi_bit_clk_upt == pinfo->mipi.dsi_bit_clk) {
		return 0;
	}

	HISI_FB_DEBUG("fb%d +.\n", hisifd->index);

	esd_enable = pinfo->esd_enable;
	if (is_mipi_video_panel(hisifd)) {
		pinfo->esd_enable = 0;
		disable_ldi(hisifd);
	}

	get_dsi_phy_ctrl(hisifd, &phy_ctrl);

	//1. wait stopstate cnt:dphy_stopstate_cnt_en=1 (pctrl_dphy_ctrl[0])

	set_reg(hisifd->pctrl_base + PERI_CTRL30, 1, 1, 0);
	set_reg(hisifd->pctrl_base + PERI_CTRL30, 1, 1, 3);
	if (is_dual_mipi_panel(hisifd)) {
		set_reg(hisifd->pctrl_base + PERI_CTRL30, 1, 1, 16);
		set_reg(hisifd->pctrl_base + PERI_CTRL30, 1, 1, 19);
	}

	is_ready = check_pctrl_trstop_flag(hisifd);

	set_reg(hisifd->pctrl_base + PERI_CTRL30, 0, 1, 0);
	if (is_dual_mipi_panel(hisifd)) {
		set_reg(hisifd->pctrl_base + PERI_CTRL30, 0, 1, 16);
	}

	if (!is_ready) {
		if (is_mipi_video_panel(hisifd)) {
			pinfo->esd_enable = esd_enable;
			enable_ldi(hisifd);
		}
		HISI_FB_DEBUG("PERI_STAT0 is not ready.\n");
		return 0;
	}

	if (is_mipi_cmd_panel(hisifd)) {
		mipi_dsi_bit_clk_upt_set_cmd(hisifd, hisifd->mipi_dsi0_base, &phy_ctrl);
		if (is_dual_mipi_panel(hisifd)) {
			mipi_dsi_bit_clk_upt_set_cmd(hisifd, hisifd->mipi_dsi1_base, &phy_ctrl);
		}
	} else {
		mipi_dsi_bit_clk_upt_set_video(hisifd, hisifd->mipi_dsi0_base, &phy_ctrl);
		if (is_dual_mipi_panel(hisifd)) {
			mipi_dsi_bit_clk_upt_set_video(hisifd, hisifd->mipi_dsi1_base, &phy_ctrl);
		}

		pinfo->esd_enable = esd_enable;
		enable_ldi(hisifd);
	}

	HISI_FB_INFO("Mipi clk successfully changed from (%d)M switch to (%d)M!\n", pinfo->mipi.dsi_bit_clk, dsi_bit_clk_upt);

	pinfo->dsi_phy_ctrl = phy_ctrl;
	pinfo->mipi.dsi_bit_clk = dsi_bit_clk_upt;

	HISI_FB_DEBUG("fb%d -.\n", hisifd->index);

	return 0;
}

