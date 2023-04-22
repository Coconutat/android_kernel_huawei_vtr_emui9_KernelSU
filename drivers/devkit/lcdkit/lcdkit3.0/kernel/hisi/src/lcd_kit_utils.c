/* Copyright (c) 2017-2018, Huawei terminal Tech. Co., Ltd. All rights reserved.
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

#include <linux/hisi/hw_cmdline_parse.h> //for runmode_is_factory
#include "global_ddr_map.h"
#include "hisi_fb.h"
#include "lcd_kit_utils.h"
#include "lcd_kit_disp.h"
#include "lcd_kit_common.h"
#include "lcd_kit_power.h"
#include "lcd_kit_parse.h"
#include "lcd_kit_adapt.h"
#include "lcd_kit_core.h"
#include "lcd_kit_effect.h"
#include "voltage/ina231.h"
#include <linux/ctype.h>

struct hisi_fb_data_type* dev_get_hisifd(struct device* dev)
{
	struct fb_info* fbi = NULL;
	struct hisi_fb_data_type* hisifd = NULL;

	if (NULL == dev) {
		LCD_KIT_ERR("lcd fps scence store dev NULL Pointer\n");
		return hisifd;
	}
	fbi = dev_get_drvdata(dev);
	if (NULL == fbi) {
		LCD_KIT_ERR("lcd fps scence store fbi NULL Pointer\n");
		return hisifd;
	}
	hisifd = (struct hisi_fb_data_type*)fbi->par;
	return hisifd;
}

bool lcd_kit_support(void)
{
	struct device_node* lcdkit_np = NULL;
	const char* support_type = NULL;
	ssize_t ret = 0;

	lcdkit_np = of_find_compatible_node(NULL, NULL, DTS_COMP_LCD_KIT_PANEL_TYPE);
	if (!lcdkit_np) {
		LCD_KIT_ERR("NOT FOUND device node!\n");
		return false;
	}
	ret = of_property_read_string(lcdkit_np, "support_lcd_type", &support_type);
	if (ret) {
		LCD_KIT_ERR("failed to get support_type.\n");
		return false;
	}
	if (!strncmp(support_type, "LCD_KIT", strlen("LCD_KIT"))) {
		LCD_KIT_INFO("lcd_kit is support!\n");
		return true;
	} else {
		LCD_KIT_INFO("lcd_kit is not support!\n");
		return false;
	}
}

static void lcd_kit_orise2x(struct hisi_panel_info* pinfo)
{
	pinfo->ifbc_cmp_dat_rev0 = 1;
	pinfo->ifbc_cmp_dat_rev1 = 0;
	pinfo->ifbc_auto_sel = 0;
}

static void lcd_kit_vesa3x_single(struct hisi_panel_info* pinfo)
{
	/* dsc parameter info */
	pinfo->vesa_dsc.bits_per_component = 8;
	pinfo->vesa_dsc.bits_per_pixel = 8;
	pinfo->vesa_dsc.initial_xmit_delay = 512;
	pinfo->vesa_dsc.first_line_bpg_offset = 12;
	pinfo->vesa_dsc.mux_word_size = 48;
	/*    DSC_CTRL */
	pinfo->vesa_dsc.block_pred_enable = 1;
	pinfo->vesa_dsc.linebuf_depth = 9;
	/* RC_PARAM3 */
	pinfo->vesa_dsc.initial_offset = 6144;
	/* FLATNESS_QP_TH */
	pinfo->vesa_dsc.flatness_min_qp = 3;
	pinfo->vesa_dsc.flatness_max_qp = 12;
	/* DSC_PARAM4 */
	pinfo->vesa_dsc.rc_edge_factor = 0x6;
	pinfo->vesa_dsc.rc_model_size = 8192;
	/* DSC_RC_PARAM5: 0x330b0b */
	pinfo->vesa_dsc.rc_tgt_offset_lo = (0x330b0b >> 20) & 0xF;
	pinfo->vesa_dsc.rc_tgt_offset_hi = (0x330b0b >> 16) & 0xF;
	pinfo->vesa_dsc.rc_quant_incr_limit1 = (0x330b0b >> 8) & 0x1F;
	pinfo->vesa_dsc.rc_quant_incr_limit0 = (0x330b0b >> 0) & 0x1F;
	/* DSC_RC_BUF_THRESH0: 0xe1c2a38 */
	pinfo->vesa_dsc.rc_buf_thresh0 = (0xe1c2a38 >> 24) & 0xFF;
	pinfo->vesa_dsc.rc_buf_thresh1 = (0xe1c2a38 >> 16) & 0xFF;
	pinfo->vesa_dsc.rc_buf_thresh2 = (0xe1c2a38 >> 8) & 0xFF;
	pinfo->vesa_dsc.rc_buf_thresh3 = (0xe1c2a38 >> 0) & 0xFF;
	/* DSC_RC_BUF_THRESH1: 0x46546269 */
	pinfo->vesa_dsc.rc_buf_thresh4 = (0x46546269 >> 24) & 0xFF;
	pinfo->vesa_dsc.rc_buf_thresh5 = (0x46546269 >> 16) & 0xFF;
	pinfo->vesa_dsc.rc_buf_thresh6 = (0x46546269 >> 8) & 0xFF;
	pinfo->vesa_dsc.rc_buf_thresh7 = (0x46546269 >> 0) & 0xFF;
	/* DSC_RC_BUF_THRESH2: 0x7077797b */
	pinfo->vesa_dsc.rc_buf_thresh8 = (0x7077797b >> 24) & 0xFF;
	pinfo->vesa_dsc.rc_buf_thresh9 = (0x7077797b >> 16) & 0xFF;
	pinfo->vesa_dsc.rc_buf_thresh10 = (0x7077797b >> 8) & 0xFF;
	pinfo->vesa_dsc.rc_buf_thresh11 = (0x7077797b >> 0) & 0xFF;
	/* DSC_RC_BUF_THRESH3: 0x7d7e0000 */
	pinfo->vesa_dsc.rc_buf_thresh12 = (0x7d7e0000 >> 24) & 0xFF;
	pinfo->vesa_dsc.rc_buf_thresh13 = (0x7d7e0000 >> 16) & 0xFF;
	/* DSC_RC_RANGE_PARAM0: 0x1020100 */
	pinfo->vesa_dsc.range_min_qp0 = (0x1020100 >> 27) & 0x1F; //lint !e572
	pinfo->vesa_dsc.range_max_qp0 = (0x1020100 >> 22) & 0x1F;
	pinfo->vesa_dsc.range_bpg_offset0 = (0x1020100 >> 16) & 0x3F;
	pinfo->vesa_dsc.range_min_qp1 = (0x1020100 >> 11) & 0x1F;
	pinfo->vesa_dsc.range_max_qp1 = (0x1020100 >> 6) & 0x1F;
	pinfo->vesa_dsc.range_bpg_offset1 = (0x1020100 >> 0) & 0x3F;
	/* DSC_RC_RANGE_PARAM1: 0x94009be */
	pinfo->vesa_dsc.range_min_qp2 = (0x94009be >> 27) & 0x1F;
	pinfo->vesa_dsc.range_max_qp2 = (0x94009be >> 22) & 0x1F;
	pinfo->vesa_dsc.range_bpg_offset2 = (0x94009be >> 16) & 0x3F;
	pinfo->vesa_dsc.range_min_qp3 = (0x94009be >> 11) & 0x1F;
	pinfo->vesa_dsc.range_max_qp3 = (0x94009be >> 6) & 0x1F;
	pinfo->vesa_dsc.range_bpg_offset3 = (0x94009be >> 0) & 0x3F;
	/* DSC_RC_RANGE_PARAM2, 0x19fc19fa */
	pinfo->vesa_dsc.range_min_qp4 = (0x19fc19fa >> 27) & 0x1F;
	pinfo->vesa_dsc.range_max_qp4 = (0x19fc19fa >> 22) & 0x1F;
	pinfo->vesa_dsc.range_bpg_offset4 = (0x19fc19fa >> 16) & 0x3F;
	pinfo->vesa_dsc.range_min_qp5 = (0x19fc19fa >> 11) & 0x1F;
	pinfo->vesa_dsc.range_max_qp5 = (0x19fc19fa >> 6) & 0x1F;
	pinfo->vesa_dsc.range_bpg_offset5 = (0x19fc19fa >> 0) & 0x3F;
	/* DSC_RC_RANGE_PARAM3, 0x19f81a38 */
	pinfo->vesa_dsc.range_min_qp6 = (0x19f81a38 >> 27) & 0x1F;
	pinfo->vesa_dsc.range_max_qp6 = (0x19f81a38 >> 22) & 0x1F;
	pinfo->vesa_dsc.range_bpg_offset6 = (0x19f81a38 >> 16) & 0x3F;
	pinfo->vesa_dsc.range_min_qp7 = (0x19f81a38 >> 11) & 0x1F;
	pinfo->vesa_dsc.range_max_qp7 = (0x19f81a38 >> 6) & 0x1F;
	pinfo->vesa_dsc.range_bpg_offset7 = (0x19f81a38 >> 0) & 0x3F;
	/* DSC_RC_RANGE_PARAM4, 0x1a781ab6 */
	pinfo->vesa_dsc.range_min_qp8 = (0x1a781ab6 >> 27) & 0x1F;
	pinfo->vesa_dsc.range_max_qp8 = (0x1a781ab6 >> 22) & 0x1F;
	pinfo->vesa_dsc.range_bpg_offset8 = (0x1a781ab6 >> 16) & 0x3F;
	pinfo->vesa_dsc.range_min_qp9 = (0x1a781ab6 >> 11) & 0x1F;
	pinfo->vesa_dsc.range_max_qp9 = (0x1a781ab6 >> 6) & 0x1F;
	pinfo->vesa_dsc.range_bpg_offset9 = (0x1a781ab6 >> 0) & 0x3F;
	/* DSC_RC_RANGE_PARAM5, 0x2af62b34 */
	pinfo->vesa_dsc.range_min_qp10 = (0x2af62b34 >> 27) & 0x1F;
	pinfo->vesa_dsc.range_max_qp10 = (0x2af62b34 >> 22) & 0x1F;
	pinfo->vesa_dsc.range_bpg_offset10 = (0x2af62b34 >> 16) & 0x3F;
	pinfo->vesa_dsc.range_min_qp11 = (0x2af62b34 >> 11) & 0x1F;
	pinfo->vesa_dsc.range_max_qp11 = (0x2af62b34 >> 6) & 0x1F;
	pinfo->vesa_dsc.range_bpg_offset11 = (0x2af62b34 >> 0) & 0x3F;
	/* DSC_RC_RANGE_PARAM6, 0x2b743b74 */
	pinfo->vesa_dsc.range_min_qp12 = (0x2b743b74 >> 27) & 0x1F;
	pinfo->vesa_dsc.range_max_qp12 = (0x2b743b74 >> 22) & 0x1F;
	pinfo->vesa_dsc.range_bpg_offset12 = (0x2b743b74 >> 16) & 0x3F;
	pinfo->vesa_dsc.range_min_qp13 = (0x2b743b74 >> 11) & 0x1F;
	pinfo->vesa_dsc.range_max_qp13 = (0x2b743b74 >> 6) & 0x1F;
	pinfo->vesa_dsc.range_bpg_offset13 = (0x2b743b74 >> 0) & 0x3F;
	/* DSC_RC_RANGE_PARAM7, 0x6bf40000 */
	pinfo->vesa_dsc.range_min_qp14 = (0x6bf40000 >> 27) & 0x1F;
	pinfo->vesa_dsc.range_max_qp14 = (0x6bf40000 >> 22) & 0x1F;
	pinfo->vesa_dsc.range_bpg_offset14 = (0x6bf40000 >> 16) & 0x3F;
}

static void lcd_kit_vesa3x_dual(struct hisi_panel_info* pinfo)
{
	pinfo->vesa_dsc.bits_per_component = 8;
	pinfo->vesa_dsc.linebuf_depth = 9;
	pinfo->vesa_dsc.bits_per_pixel = 8;
	pinfo->vesa_dsc.initial_xmit_delay = 512;
	pinfo->vesa_dsc.first_line_bpg_offset = 12;
	pinfo->vesa_dsc.mux_word_size = 48;
	/* DSC_CTRL */
	pinfo->vesa_dsc.block_pred_enable = 1;//0;
	/* RC_PARAM3 */
	pinfo->vesa_dsc.initial_offset = 6144;
	/* FLATNESS_QP_TH */
	pinfo->vesa_dsc.flatness_min_qp = 3;
	pinfo->vesa_dsc.flatness_max_qp = 12;
	/* DSC_PARAM4 */
	pinfo->vesa_dsc.rc_edge_factor = 0x6;
	pinfo->vesa_dsc.rc_model_size = 8192;
	/* DSC_RC_PARAM5: 0x330b0b */
	pinfo->vesa_dsc.rc_tgt_offset_lo = (0x330b0b >> 20) & 0xF;
	pinfo->vesa_dsc.rc_tgt_offset_hi = (0x330b0b >> 16) & 0xF;
	pinfo->vesa_dsc.rc_quant_incr_limit1 = (0x330b0b >> 8) & 0x1F;
	pinfo->vesa_dsc.rc_quant_incr_limit0 = (0x330b0b >> 0) & 0x1F;
	/* DSC_RC_BUF_THRESH0: 0xe1c2a38 */
	pinfo->vesa_dsc.rc_buf_thresh0 = (0xe1c2a38 >> 24) & 0xFF;
	pinfo->vesa_dsc.rc_buf_thresh1 = (0xe1c2a38 >> 16) & 0xFF;
	pinfo->vesa_dsc.rc_buf_thresh2 = (0xe1c2a38 >> 8) & 0xFF;
	pinfo->vesa_dsc.rc_buf_thresh3 = (0xe1c2a38 >> 0) & 0xFF;
	/* DSC_RC_BUF_THRESH1: 0x46546269 */
	pinfo->vesa_dsc.rc_buf_thresh4 = (0x46546269 >> 24) & 0xFF;
	pinfo->vesa_dsc.rc_buf_thresh5 = (0x46546269 >> 16) & 0xFF;
	pinfo->vesa_dsc.rc_buf_thresh6 = (0x46546269 >> 8) & 0xFF;
	pinfo->vesa_dsc.rc_buf_thresh7 = (0x46546269 >> 0) & 0xFF;
	/* DSC_RC_BUF_THRESH2: 0x7077797b */
	pinfo->vesa_dsc.rc_buf_thresh8 = (0x7077797b >> 24) & 0xFF;
	pinfo->vesa_dsc.rc_buf_thresh9 = (0x7077797b >> 16) & 0xFF;
	pinfo->vesa_dsc.rc_buf_thresh10 = (0x7077797b >> 8) & 0xFF;
	pinfo->vesa_dsc.rc_buf_thresh11 = (0x7077797b >> 0) & 0xFF;
	/* DSC_RC_BUF_THRESH3: 0x7d7e0000 */
	pinfo->vesa_dsc.rc_buf_thresh12 = (0x7d7e0000 >> 24) & 0xFF;
	pinfo->vesa_dsc.rc_buf_thresh13 = (0x7d7e0000 >> 16) & 0xFF;
	/* DSC_RC_RANGE_PARAM0: 0x1020100 */
	pinfo->vesa_dsc.range_min_qp0 = (0x1020100 >> 27) & 0x1F; //lint !e572
	pinfo->vesa_dsc.range_max_qp0 = (0x1020100 >> 22) & 0x1F;
	pinfo->vesa_dsc.range_bpg_offset0 = (0x1020100 >> 16) & 0x3F;
	pinfo->vesa_dsc.range_min_qp1 = (0x1020100 >> 11) & 0x1F;
	pinfo->vesa_dsc.range_max_qp1 = (0x1020100 >> 6) & 0x1F;
	pinfo->vesa_dsc.range_bpg_offset1 = (0x1020100 >> 0) & 0x3F;
	/* DSC_RC_RANGE_PARAM1: 0x94009be */
	pinfo->vesa_dsc.range_min_qp2 = (0x94009be >> 27) & 0x1F;
	pinfo->vesa_dsc.range_max_qp2 = (0x94009be >> 22) & 0x1F;
	pinfo->vesa_dsc.range_bpg_offset2 = (0x94009be >> 16) & 0x3F;
	pinfo->vesa_dsc.range_min_qp3 = (0x94009be >> 11) & 0x1F;
	pinfo->vesa_dsc.range_max_qp3 = (0x94009be >> 6) & 0x1F;
	pinfo->vesa_dsc.range_bpg_offset3 = (0x94009be >> 0) & 0x3F;
	/* DSC_RC_RANGE_PARAM2, 0x19fc19fa */
	pinfo->vesa_dsc.range_min_qp4 = (0x19fc19fa >> 27) & 0x1F;
	pinfo->vesa_dsc.range_max_qp4 = (0x19fc19fa >> 22) & 0x1F;
	pinfo->vesa_dsc.range_bpg_offset4 = (0x19fc19fa >> 16) & 0x3F;
	pinfo->vesa_dsc.range_min_qp5 = (0x19fc19fa >> 11) & 0x1F;
	pinfo->vesa_dsc.range_max_qp5 = (0x19fc19fa >> 6) & 0x1F;
	pinfo->vesa_dsc.range_bpg_offset5 = (0x19fc19fa >> 0) & 0x3F;
	/* DSC_RC_RANGE_PARAM3, 0x19f81a38 */
	pinfo->vesa_dsc.range_min_qp6 = (0x19f81a38 >> 27) & 0x1F;
	pinfo->vesa_dsc.range_max_qp6 = (0x19f81a38 >> 22) & 0x1F;
	pinfo->vesa_dsc.range_bpg_offset6 = (0x19f81a38 >> 16) & 0x3F;
	pinfo->vesa_dsc.range_min_qp7 = (0x19f81a38 >> 11) & 0x1F;
	pinfo->vesa_dsc.range_max_qp7 = (0x19f81a38 >> 6) & 0x1F;
	pinfo->vesa_dsc.range_bpg_offset7 = (0x19f81a38 >> 0) & 0x3F;
	/* DSC_RC_RANGE_PARAM4, 0x1a781ab6 */
	pinfo->vesa_dsc.range_min_qp8 = (0x1a781ab6 >> 27) & 0x1F;
	pinfo->vesa_dsc.range_max_qp8 = (0x1a781ab6 >> 22) & 0x1F;
	pinfo->vesa_dsc.range_bpg_offset8 = (0x1a781ab6 >> 16) & 0x3F;
	pinfo->vesa_dsc.range_min_qp9 = (0x1a781ab6 >> 11) & 0x1F;
	pinfo->vesa_dsc.range_max_qp9 = (0x1a781ab6 >> 6) & 0x1F;
	pinfo->vesa_dsc.range_bpg_offset9 = (0x1a781ab6 >> 0) & 0x3F;
	/* DSC_RC_RANGE_PARAM5, 0x2af62b34 */
	pinfo->vesa_dsc.range_min_qp10 = (0x2af62b34 >> 27) & 0x1F;
	pinfo->vesa_dsc.range_max_qp10 = (0x2af62b34 >> 22) & 0x1F;
	pinfo->vesa_dsc.range_bpg_offset10 = (0x2af62b34 >> 16) & 0x3F;
	pinfo->vesa_dsc.range_min_qp11 = (0x2af62b34 >> 11) & 0x1F;
	pinfo->vesa_dsc.range_max_qp11 = (0x2af62b34 >> 6) & 0x1F;
	pinfo->vesa_dsc.range_bpg_offset11 = (0x2af62b34 >> 0) & 0x3F;
	/* DSC_RC_RANGE_PARAM6, 0x2b743b74 */
	pinfo->vesa_dsc.range_min_qp12 = (0x2b743b74 >> 27) & 0x1F;
	pinfo->vesa_dsc.range_max_qp12 = (0x2b743b74 >> 22) & 0x1F;
	pinfo->vesa_dsc.range_bpg_offset12 = (0x2b743b74 >> 16) & 0x3F;
	pinfo->vesa_dsc.range_min_qp13 = (0x2b743b74 >> 11) & 0x1F;
	pinfo->vesa_dsc.range_max_qp13 = (0x2b743b74 >> 6) & 0x1F;
	pinfo->vesa_dsc.range_bpg_offset13 = (0x2b743b74 >> 0) & 0x3F;
	/* DSC_RC_RANGE_PARAM7, 0x6bf40000 */
	pinfo->vesa_dsc.range_min_qp14 = (0x6bf40000 >> 27) & 0x1F;
	pinfo->vesa_dsc.range_max_qp14 = (0x6bf40000 >> 22) & 0x1F;
	pinfo->vesa_dsc.range_bpg_offset14 = (0x6bf40000 >> 16) & 0x3F;
}

static void lcd_kit_vesa3_75x_dual(struct hisi_panel_info* pinfo)
{
	pinfo->vesa_dsc.bits_per_component = 10;
	pinfo->vesa_dsc.linebuf_depth = 11;
	pinfo->vesa_dsc.bits_per_pixel = 8;
	pinfo->vesa_dsc.initial_xmit_delay = 512;
	pinfo->vesa_dsc.first_line_bpg_offset = 12;
	pinfo->vesa_dsc.mux_word_size = 48;
	/* DSC_CTRL */
	pinfo->vesa_dsc.block_pred_enable = 1;//0;
	/* RC_PARAM3 */
	pinfo->vesa_dsc.initial_offset = 6144;
	/* FLATNESS_QP_TH */
	pinfo->vesa_dsc.flatness_min_qp = 7;
	pinfo->vesa_dsc.flatness_max_qp = 16;
	/* DSC_PARAM4 */
	pinfo->vesa_dsc.rc_edge_factor = 0x6;
	pinfo->vesa_dsc.rc_model_size = 8192;
	/* DSC_RC_PARAM5: 0x330f0f */
	pinfo->vesa_dsc.rc_tgt_offset_lo = (0x330f0f >> 20) & 0xF;
	pinfo->vesa_dsc.rc_tgt_offset_hi = (0x330f0f >> 16) & 0xF;
	pinfo->vesa_dsc.rc_quant_incr_limit1 = (0x330f0f >> 8) & 0x1F;
	pinfo->vesa_dsc.rc_quant_incr_limit0 = (0x330f0f >> 0) & 0x1F;
	/* DSC_RC_BUF_THRESH0: 0xe1c2a38 */
	pinfo->vesa_dsc.rc_buf_thresh0 = (0xe1c2a38 >> 24) & 0xFF;
	pinfo->vesa_dsc.rc_buf_thresh1 = (0xe1c2a38 >> 16) & 0xFF;
	pinfo->vesa_dsc.rc_buf_thresh2 = (0xe1c2a38 >> 8) & 0xFF;
	pinfo->vesa_dsc.rc_buf_thresh3 = (0xe1c2a38 >> 0) & 0xFF;
	/* DSC_RC_BUF_THRESH1: 0x46546269 */
	pinfo->vesa_dsc.rc_buf_thresh4 = (0x46546269 >> 24) & 0xFF;
	pinfo->vesa_dsc.rc_buf_thresh5 = (0x46546269 >> 16) & 0xFF;
	pinfo->vesa_dsc.rc_buf_thresh6 = (0x46546269 >> 8) & 0xFF;
	pinfo->vesa_dsc.rc_buf_thresh7 = (0x46546269 >> 0) & 0xFF;
	/* DSC_RC_BUF_THRESH2: 0x7077797b */
	pinfo->vesa_dsc.rc_buf_thresh8 = (0x7077797b >> 24) & 0xFF;
	pinfo->vesa_dsc.rc_buf_thresh9 = (0x7077797b >> 16) & 0xFF;
	pinfo->vesa_dsc.rc_buf_thresh10 = (0x7077797b >> 8) & 0xFF;
	pinfo->vesa_dsc.rc_buf_thresh11 = (0x7077797b >> 0) & 0xFF;
	/* DSC_RC_BUF_THRESH3: 0x7d7e0000 */
	pinfo->vesa_dsc.rc_buf_thresh12 = (0x7d7e0000 >> 24) & 0xFF;
	pinfo->vesa_dsc.rc_buf_thresh13 = (0x7d7e0000 >> 16) & 0xFF;
	/* DSC_RC_RANGE_PARAM0: 0x2022200 */
	pinfo->vesa_dsc.range_min_qp0 = (0x2022200 >> 27) & 0x1F; //lint !e572
	pinfo->vesa_dsc.range_max_qp0 = (0x2022200 >> 22) & 0x1F;
	pinfo->vesa_dsc.range_bpg_offset0 = (0x2022200 >> 16) & 0x3F;
	pinfo->vesa_dsc.range_min_qp1 = (0x2022200 >> 11) & 0x1F;
	pinfo->vesa_dsc.range_max_qp1 = (0x2022200 >> 6) & 0x1F;
	pinfo->vesa_dsc.range_bpg_offset1 = (0x2022200 >> 0) & 0x3F;
	/* DSC_RC_RANGE_PARAM1: 0x94009be */
	pinfo->vesa_dsc.range_min_qp2 = 5;//(0x94009be >> 27) & 0x1F;
	pinfo->vesa_dsc.range_max_qp2 = 9;//(0x94009be >> 22) & 0x1F;
	pinfo->vesa_dsc.range_bpg_offset2 = (0x94009be >> 16) & 0x3F;
	pinfo->vesa_dsc.range_min_qp3 = 5;//(0x94009be >> 11) & 0x1F;
	pinfo->vesa_dsc.range_max_qp3 = 10;//(0x94009be >> 6) & 0x1F;
	pinfo->vesa_dsc.range_bpg_offset3 = (0x94009be >> 0) & 0x3F;
	/* DSC_RC_RANGE_PARAM2, 0x19fc19fa */
	pinfo->vesa_dsc.range_min_qp4 = 7;//(0x19fc19fa >> 27) & 0x1F;
	pinfo->vesa_dsc.range_max_qp4 = 11;//(0x19fc19fa >> 22) & 0x1F;
	pinfo->vesa_dsc.range_bpg_offset4 = (0x19fc19fa >> 16) & 0x3F;
	pinfo->vesa_dsc.range_min_qp5 = 7;//(0x19fc19fa >> 11) & 0x1F;
	pinfo->vesa_dsc.range_max_qp5 = 11;//(0x19fc19fa >> 6) & 0x1F;
	pinfo->vesa_dsc.range_bpg_offset5 = (0x19fc19fa >> 0) & 0x3F;
	/* DSC_RC_RANGE_PARAM3, 0x19f81a38 */
	pinfo->vesa_dsc.range_min_qp6 = 7;//(0x19f81a38 >> 27) & 0x1F;
	pinfo->vesa_dsc.range_max_qp6 = 11;//(0x19f81a38 >> 22) & 0x1F;
	pinfo->vesa_dsc.range_bpg_offset6 = (0x19f81a38 >> 16) & 0x3F;
	pinfo->vesa_dsc.range_min_qp7 = 7;//(0x19f81a38 >> 11) & 0x1F;
	pinfo->vesa_dsc.range_max_qp7 = 12;//(0x19f81a38 >> 6) & 0x1F;
	pinfo->vesa_dsc.range_bpg_offset7 = (0x19f81a38 >> 0) & 0x3F;
	/* DSC_RC_RANGE_PARAM4, 0x1a781ab6 */
	pinfo->vesa_dsc.range_min_qp8 = 7;//(0x1a781ab6 >> 27) & 0x1F;
	pinfo->vesa_dsc.range_max_qp8 = 13;//(0x1a781ab6 >> 22) & 0x1F;
	pinfo->vesa_dsc.range_bpg_offset8 = (0x1a781ab6 >> 16) & 0x3F;
	pinfo->vesa_dsc.range_min_qp9 = 7;//(0x1a781ab6 >> 11) & 0x1F;
	pinfo->vesa_dsc.range_max_qp9 = 14;//(0x1a781ab6 >> 6) & 0x1F;
	pinfo->vesa_dsc.range_bpg_offset9 = (0x1a781ab6 >> 0) & 0x3F;
	/* DSC_RC_RANGE_PARAM5, 0x2af62b34 */
	pinfo->vesa_dsc.range_min_qp10 = 9;//(0x2af62b34 >> 27) & 0x1F;
	pinfo->vesa_dsc.range_max_qp10 = 15;//(0x2af62b34 >> 22) & 0x1F;
	pinfo->vesa_dsc.range_bpg_offset10 = (0x2af62b34 >> 16) & 0x3F;
	pinfo->vesa_dsc.range_min_qp11 = 9;//(0x2af62b34 >> 11) & 0x1F;
	pinfo->vesa_dsc.range_max_qp11 = 16;//(0x2af62b34 >> 6) & 0x1F;
	pinfo->vesa_dsc.range_bpg_offset11 = (0x2af62b34 >> 0) & 0x3F;
	/* DSC_RC_RANGE_PARAM6, 0x2b743b74 */
	pinfo->vesa_dsc.range_min_qp12 = 9;//(0x2b743b74 >> 27) & 0x1F;
	pinfo->vesa_dsc.range_max_qp12 = 17;//(0x2b743b74 >> 22) & 0x1F;
	pinfo->vesa_dsc.range_bpg_offset12 = (0x2b743b74 >> 16) & 0x3F;
	pinfo->vesa_dsc.range_min_qp13 = 11;//(0x2b743b74 >> 11) & 0x1F;
	pinfo->vesa_dsc.range_max_qp13 = 17;//(0x2b743b74 >> 6) & 0x1F;
	pinfo->vesa_dsc.range_bpg_offset13 = (0x2b743b74 >> 0) & 0x3F;
	/* DSC_RC_RANGE_PARAM7, 0x6bf40000 */
	pinfo->vesa_dsc.range_min_qp14 = 17;//(0x6bf40000 >> 27) & 0x1F;
	pinfo->vesa_dsc.range_max_qp14 = 19;//(0x6bf40000 >> 22) & 0x1F;
	pinfo->vesa_dsc.range_bpg_offset14 = (0x6bf40000 >> 16) & 0x3F;
}

void lcd_kit_compress_config(int mode, struct hisi_panel_info* pinfo)
{
	if (!pinfo) {
		LCD_KIT_ERR("pinfo is null\n");
		return ;
	}
	switch (mode) {
		case IFBC_TYPE_ORISE2X:
			lcd_kit_orise2x(pinfo);
			break;
		case IFBC_TYPE_VESA3X_SINGLE:
			lcd_kit_vesa3x_single(pinfo);
			break;
		case IFBC_TYPE_VESA3_75X_DUAL:
			lcd_kit_vesa3_75x_dual(pinfo);
			break;
		case IFBC_TYPE_VESA3X_DUAL:
			lcd_kit_vesa3x_dual(pinfo);
			break;
		case IFBC_TYPE_NONE:
			break;
		default:
			LCD_KIT_ERR("not support compress mode:%d\n", mode);
			break;
	}
}

int lcd_kit_lread_reg(void* pdata, uint32_t* out, struct lcd_kit_dsi_cmd_desc* cmds, uint32_t len)
{
	int ret = LCD_KIT_OK;
	struct dsi_cmd_desc lcd_reg_cmd;
	struct hisi_fb_data_type* hisifd = NULL;

	hisifd = (struct hisi_fb_data_type*) pdata;
	lcd_reg_cmd.dtype = cmds->dtype;
	lcd_reg_cmd.vc = cmds->vc;
	lcd_reg_cmd.wait = cmds->wait;
	lcd_reg_cmd.waittype = cmds->waittype;
	lcd_reg_cmd.dlen = cmds->dlen;
	lcd_reg_cmd.payload = cmds->payload;
	ret = mipi_dsi_lread_reg(out, &lcd_reg_cmd, len, hisifd->mipi_dsi0_base);
	if (ret) {
		LCD_KIT_INFO("read error, ret=%d\n", ret);
		return ret;
	}
	return ret;
}

#define PROJECTID_LEN 9
#define PROJECTID_PRD_LEN 4
static int lcd_kit_check_project_id(void)
{
	int i = 0;

	for (; i < PROJECTID_PRD_LEN; i++) {
		if (isalpha((disp_info->project_id.id)[i]) == 0) {
			return LCD_KIT_FAIL;
		}
	}
	for (; i < PROJECTID_LEN; i++) {
		if (isdigit((disp_info->project_id.id)[i]) == 0) {
			return LCD_KIT_FAIL;
		}
	}
	return LCD_KIT_OK;
}

int lcd_kit_read_project_id(void)
{
	struct hisi_fb_data_type* hisifd = NULL;
	struct lcd_kit_panel_ops * panel_ops = NULL;

	if (disp_info->project_id.support == 0) {
		return LCD_KIT_OK;
	}

	memset(disp_info->project_id.id, 0, sizeof(disp_info->project_id.id));
	panel_ops = lcd_kit_panel_get_ops();
	if (panel_ops && panel_ops->lcd_kit_read_project_id) {
		return panel_ops->lcd_kit_read_project_id();
	}

	hisifd = hisifd_list[PRIMARY_PANEL_IDX];
	if (hisifd == NULL) {
		LCD_KIT_ERR("hisifd is null\n");
		return LCD_KIT_FAIL;
	}

	if (LCD_KIT_OK == lcd_kit_dsi_cmds_rx(hisifd, (uint8_t*)disp_info->project_id.id, &disp_info->project_id.cmds)
			&& LCD_KIT_OK == lcd_kit_check_project_id()) {
		LCD_KIT_INFO("read project id is %s\n", disp_info->project_id.id);
		return LCD_KIT_OK;
	}
	if (disp_info->project_id.default_project_id) {
		strncpy(disp_info->project_id.id, disp_info->project_id.default_project_id, PROJECTID_LEN+1);
		LCD_KIT_ERR("use default project id:%s\n", disp_info->project_id.default_project_id);
	}
	return LCD_KIT_FAIL;
}

int lcd_kit_rgbw_set_mode(struct hisi_fb_data_type* hisifd, int mode)
{
	int ret = LCD_KIT_OK;
	static int old_rgbw_mode = 0;
	int rgbw_mode = hisifd->de_info.ddic_rgbw_mode;
	struct lcd_kit_panel_ops *panel_ops = NULL;

	panel_ops = lcd_kit_panel_get_ops();
	if (panel_ops && panel_ops->lcd_kit_rgbw_set_mode) {
		ret = panel_ops->lcd_kit_rgbw_set_mode(hisifd, hisifd->de_info.ddic_rgbw_mode);
	}
	else if (rgbw_mode != old_rgbw_mode) {
		switch (mode) {
			case RGBW_SET1_MODE:
				ret = lcd_kit_dsi_cmds_tx(hisifd, &disp_info->rgbw.mode1_cmds);
				break;
			case RGBW_SET2_MODE:
				ret = lcd_kit_dsi_cmds_tx(hisifd, &disp_info->rgbw.mode2_cmds);
				break;
			case RGBW_SET3_MODE:
				ret = lcd_kit_dsi_cmds_tx(hisifd, &disp_info->rgbw.mode3_cmds);
				break;
			case RGBW_SET4_MODE:
				ret = lcd_kit_dsi_cmds_tx(hisifd, &disp_info->rgbw.mode4_cmds);
				break;
			default:
				HISI_FB_ERR("mode err: %d\n", hisifd->de_info.ddic_rgbw_mode);
				ret = -1;
				break;
		}
	}
	LCD_KIT_DEBUG("[RGBW] rgbw_mode=%d,rgbw_mode_old=%d!\n",rgbw_mode,old_rgbw_mode);
	old_rgbw_mode = rgbw_mode;

	return ret;
}

int lcd_kit_rgbw_set_backlight(struct hisi_fb_data_type* hisifd, int bl_level)
{
	int ret = LCD_KIT_OK;

	disp_info->rgbw.backlight_cmds.cmds[0].payload[1] = (bl_level >> 8) & 0xff;
	disp_info->rgbw.backlight_cmds.cmds[0].payload[2] = bl_level & 0xff;
	ret = lcd_kit_dsi_cmds_tx(hisifd, &disp_info->rgbw.backlight_cmds);
	return ret;
}

int lcd_kit_rgbw_set_handle(struct hisi_fb_data_type* hisifd)
{
	int ret = LCD_KIT_OK;
	static int old_rgbw_backlight = 0;
	int rgbw_backlight = 0;
	int rgbw_bl_level = 0;

	/*set mode*/
	ret = lcd_kit_rgbw_set_mode(hisifd, hisifd->de_info.ddic_rgbw_mode);
	if (ret) {
		LCD_KIT_ERR("[RGBW]set mode fail\n");
		return LCD_KIT_FAIL;
	}

	/*set backlight*/
	rgbw_backlight = hisifd->de_info.ddic_rgbw_backlight;
	if (disp_info->rgbw.backlight_cmds.cmds && (hisifd->bl_level && (hisifd->backlight.bl_level_old != 0)) \
                                                                 && (rgbw_backlight != old_rgbw_backlight)) {
		rgbw_bl_level = rgbw_backlight * disp_info->rgbw.rgbw_bl_max / hisifd->panel_info.bl_max;
		ret = lcd_kit_rgbw_set_backlight(hisifd, rgbw_bl_level);
		if (ret) {
			LCD_KIT_ERR("[RGBW]set backlight fail\n");
			return LCD_KIT_FAIL;
		}
	}
	old_rgbw_backlight = rgbw_backlight;

	return ret;
}

uint8_t g_last_fps_scence = LCD_FPS_SCENCE_NORMAL;
int lcd_kit_updt_fps(struct platform_device* pdev)
{
	int ret = LCD_KIT_OK;
	struct hisi_fb_data_type* hisifd = NULL;
	struct hisi_panel_info* pinfo = NULL;

	hisifd = platform_get_drvdata(pdev);
	if (NULL == hisifd) {
		LCD_KIT_ERR("hisifd is null\n");
		return LCD_KIT_FAIL;
	}
	pinfo = &(hisifd->panel_info);
	if (pinfo == NULL) {
		LCD_KIT_ERR();
		return LCD_KIT_FAIL;
	}
	if(g_last_fps_scence == pinfo->fps_scence)
	{
		LCD_KIT_DEBUG("The scence is same for ddic and it needn't to send fps cmds to ddic!\n");
		return ret;
	}
	switch (pinfo->fps_scence) {
		case LCD_FPS_SCENCE_FUNC_DEFAULT_DISABLE:
			ret = lcd_kit_dsi_cmds_tx_no_lock(hisifd, &disp_info->fps.fps_to_60_cmds);
			pinfo->fps_updt_support = 0;
			pinfo->fps_updt_panel_only = 0;
			g_last_fps_scence = LCD_FPS_SCENCE_NORMAL;
			break;
		case LCD_FPS_SCENCE_FUNC_DEFAULT_ENABLE:
			ret = lcd_kit_dsi_cmds_tx_no_lock(hisifd, &disp_info->fps.dfr_enable_cmds);
			g_last_fps_scence = LCD_FPS_SCENCE_FUNC_DEFAULT_ENABLE;
			break;
		case LCD_FPS_SCENCE_FORCE_30FPS:
			ret = lcd_kit_dsi_cmds_tx_no_lock(hisifd, &disp_info->fps.fps_to_30_cmds);
			g_last_fps_scence = LCD_FPS_SCENCE_FORCE_30FPS;
			break;
		default:
			break;
	}
	if (pinfo->fps_updt_force_update) {
		LCD_KIT_INFO("set fps_updt_force_update = 0\n");
		pinfo->fps_updt_force_update = 0;
	}
	return ret;
}

void lcd_kit_fps_scence_set(struct hisi_panel_info* pinfo, uint32_t scence)
{
	switch (scence) {
		case LCD_FPS_SCENCE_NORMAL:
			pinfo->fps_updt = LCD_FPS_60;
			break;
		case LCD_FPS_SCENCE_IDLE:
			pinfo->fps_updt = LCD_FPS_30;
			break;
		case LCD_FPS_SCENCE_FORCE_30FPS:
			pinfo->fps_updt_support = 1;
			pinfo->fps_updt_panel_only = 1;
			pinfo->fps_updt = LCD_FPS_30;
			pinfo->fps_updt_force_update = 1;
			pinfo->fps_scence = scence;
			break;
		case LCD_FPS_SCENCE_FUNC_DEFAULT_ENABLE:
			pinfo->fps_updt_support = 1;
			pinfo->fps_updt_panel_only = 0;
			pinfo->fps_updt = LCD_FPS_60;
			pinfo->fps_updt_force_update = 1;
			pinfo->fps_scence = scence;
			break;
		case LCD_FPS_SCENCE_FUNC_DEFAULT_DISABLE:
			pinfo->fps_updt_force_update = 1;
			pinfo->fps_updt = LCD_FPS_60;
			pinfo->fps_scence = scence;
			break;
		default:
			pinfo->fps_updt = LCD_FPS_60;
			break;
	}
}

void lcd_kit_fps_updt_porch(struct hisi_panel_info* pinfo, uint32_t scence)
{
	if (!pinfo) {
		LCD_KIT_ERR("pinfo is null\n");
		return ;
	}
	switch (scence) {
		case LCD_KIT_FPS_SCENCE_IDLE:
			pinfo->ldi_updt.h_back_porch = disp_info->fps.low_frame_porch.buf[0];
			pinfo->ldi_updt.h_front_porch = disp_info->fps.low_frame_porch.buf[1];
			pinfo->ldi_updt.h_pulse_width = disp_info->fps.low_frame_porch.buf[2];
			pinfo->ldi_updt.v_back_porch = disp_info->fps.low_frame_porch.buf[3];
			pinfo->ldi_updt.v_front_porch = disp_info->fps.low_frame_porch.buf[4];
			pinfo->ldi_updt.v_pulse_width = disp_info->fps.low_frame_porch.buf[5];
			break;
		case LCD_KIT_FPS_SCENCE_EBOOK:
			pinfo->ldi_updt.h_back_porch = disp_info->fps.low_frame_porch.buf[0];
			pinfo->ldi_updt.h_front_porch = disp_info->fps.low_frame_porch.buf[1];
			pinfo->ldi_updt.h_pulse_width = disp_info->fps.low_frame_porch.buf[2];
			pinfo->ldi_updt.v_back_porch = disp_info->fps.low_frame_porch.buf[3];
			pinfo->ldi_updt.v_front_porch = disp_info->fps.low_frame_porch.buf[4];
			pinfo->ldi_updt.v_pulse_width = disp_info->fps.low_frame_porch.buf[5];
			break;
		default:
			pinfo->ldi_updt.h_back_porch = disp_info->fps.normal_frame_porch.buf[0];
			pinfo->ldi_updt.h_front_porch = disp_info->fps.normal_frame_porch.buf[1];
			pinfo->ldi_updt.h_pulse_width = disp_info->fps.normal_frame_porch.buf[2];
			pinfo->ldi_updt.v_back_porch = disp_info->fps.normal_frame_porch.buf[3];
			pinfo->ldi_updt.v_front_porch = disp_info->fps.normal_frame_porch.buf[4];
			pinfo->ldi_updt.v_pulse_width = disp_info->fps.normal_frame_porch.buf[5];
			break;
	}
}

int lcd_kit_updt_fps_scence(struct platform_device* pdev, uint32_t scence)
{
	int ret = LCD_KIT_OK;
	struct hisi_fb_data_type* hisifd = NULL;

	if (NULL == pdev) {
		LCD_KIT_ERR("pdev is null\n");
		return LCD_KIT_FAIL;
	}
	hisifd = platform_get_drvdata(pdev);
	if (NULL == hisifd) {
		LCD_KIT_ERR("hisifd is null\n");
		return LCD_KIT_FAIL;
	}

	lcd_kit_fps_scence_set(&hisifd->panel_info, scence);
	if (is_mipi_video_panel(hisifd)) {
		lcd_kit_fps_updt_porch(&hisifd->panel_info, scence);
	}
	return ret;
}

void lcd_kit_disp_on_check_delay(void)
{
	long delta_time_backlight_to_panel_on = 0;
	u32 delay_margin = 0;
	struct timeval tv;

	memset(&tv, 0, sizeof(struct timeval));
	do_gettimeofday(&tv);
	LCD_KIT_INFO("set backlight at %lu seconds %lu mil seconds\n",
				 tv.tv_sec, tv.tv_usec);
	delta_time_backlight_to_panel_on = (tv.tv_sec - disp_info->quickly_sleep_out.panel_on_record_tv.tv_sec) * 1000000
									   + tv.tv_usec - disp_info->quickly_sleep_out.panel_on_record_tv.tv_usec;
	delta_time_backlight_to_panel_on /= 1000;
	if (delta_time_backlight_to_panel_on >= disp_info->quickly_sleep_out.interval) {
		LCD_KIT_INFO("%lu > %d, no need delay\n",
					 delta_time_backlight_to_panel_on,
					 disp_info->quickly_sleep_out.interval);
		goto CHECK_DELAY_END;
	}
	delay_margin = disp_info->quickly_sleep_out.interval - delta_time_backlight_to_panel_on;
	if (delay_margin > 200) {
		LCD_KIT_INFO("something maybe error");
		goto CHECK_DELAY_END;
	}
	msleep(delay_margin);
CHECK_DELAY_END:
	disp_info->quickly_sleep_out.panel_on_tag = false;
	return;
}

void lcd_kit_disp_on_record_time(void)
{
	do_gettimeofday(&disp_info->quickly_sleep_out.panel_on_record_tv);
	LCD_KIT_INFO("display on at %lu seconds %lu mil seconds\n", disp_info->quickly_sleep_out.panel_on_record_tv.tv_sec,
				 disp_info->quickly_sleep_out.panel_on_record_tv.tv_usec);
	disp_info->quickly_sleep_out.panel_on_tag = true;
	return;
}


int lcd_kit_get_bl_set_type(struct hisi_panel_info* pinfo)
{
	if (pinfo->bl_set_type & BL_SET_BY_PWM) {
		return BL_SET_BY_PWM;
	} else if (pinfo->bl_set_type & BL_SET_BY_BLPWM) {
		return BL_SET_BY_BLPWM;
	} else if (pinfo->bl_set_type & BL_SET_BY_MIPI) {
		return BL_SET_BY_MIPI;
	} else {
		return BL_SET_BY_NONE;
	}
}

int lcd_kit_alpm_setting(struct hisi_fb_data_type* hisifd, uint32_t mode)
{
	int ret = LCD_KIT_OK;

	if (hisifd == NULL) {
		HISI_FB_ERR("hisifd is null\n");
		return LCD_KIT_FAIL;
	}
	LCD_KIT_INFO("AOD mode is %d \n", mode);
	switch (mode) {
		case ALPM_DISPLAY_OFF:
			hisifd->aod_mode = 1;
			mutex_lock(&disp_info->mipi_lock);
			if (common_info->esd.support) {
				common_info->esd.status = ESD_STOP;
			}
			mutex_lock(&disp_info->mipi_lock);
			ret = lcd_kit_dsi_cmds_tx(hisifd, &disp_info->alpm.off_cmds);
			break;
		case ALPM_ON_MIDDLE_LIGHT:
			ret = lcd_kit_dsi_cmds_tx(hisifd, &disp_info->alpm.high_light_cmds);
			mutex_lock(&disp_info->mipi_lock);
			if (common_info->esd.support) {
				common_info->esd.status = ESD_RUNNING;
			}
			mutex_lock(&disp_info->mipi_lock);
			break;
		case ALPM_EXIT:
			ret = lcd_kit_dsi_cmds_tx(hisifd, &disp_info->alpm.exit_cmds);
			mutex_lock(&disp_info->mipi_lock);
			if (common_info->esd.support) {
				common_info->esd.status = ESD_RUNNING;
			}
			mutex_lock(&disp_info->mipi_lock);
			hisifd->aod_mode = 0;
			break;
		case ALPM_ON_LOW_LIGHT:
			ret = lcd_kit_dsi_cmds_tx(hisifd, &disp_info->alpm.low_light_cmds);
			mutex_lock(&disp_info->mipi_lock);
			if (common_info->esd.support) {
				common_info->esd.status = ESD_RUNNING;
			}
			mutex_lock(&disp_info->mipi_lock);
			break;
		default:
			break;
	}
	return ret;
}

int lcd_kit_rgbw_set_bl(struct hisi_fb_data_type* hisifd, uint32_t level)
{
	uint32_t rgbw_level = 0;
	int ret = LCD_KIT_OK;

	rgbw_level = level * disp_info->rgbw.rgbw_bl_max / hisifd->panel_info.bl_max;
	disp_info->rgbw.backlight_cmds.cmds[0].payload[1] = (rgbw_level >> 8) & 0xff;
	disp_info->rgbw.backlight_cmds.cmds[0].payload[2] = rgbw_level & 0xff;
	hisifb_activate_vsync(hisifd);
	ret = lcd_kit_dsi_cmds_tx(hisifd, &disp_info->rgbw.backlight_cmds);
	hisifb_deactivate_vsync(hisifd);
	//delay 2 frame time
	msleep(38);
	return ret;
}

int lcd_kit_blpwm_set_backlight(struct hisi_fb_data_type* hisifd, uint32_t level)
{
	uint32_t bl_level = 0;
	static uint32_t last_bl_level = 255;
	uint32_t ret = LCD_KIT_OK;
	struct hisi_panel_info* pinfo = NULL;

	pinfo = &(hisifd->panel_info);
	if (!pinfo) {
		LCD_KIT_ERR("pinfo is null\n");
		return LCD_KIT_FAIL;
	}
	bl_level = (level < hisifd->panel_info.bl_max) ? level : hisifd->panel_info.bl_max;
	if (disp_info->rgbw.support && disp_info->rgbw.backlight_cmds.cmds) {
		if (hisifd->backlight.bl_level_old == 0 && level != 0) {
			ret = lcd_kit_rgbw_set_bl(hisifd, bl_level);
			if (ret) {
				LCD_KIT_ERR("set rgbw level fail\n");
			}
		}
	}
	ret = hisi_blpwm_set_bl(hisifd, bl_level);
	if (power_hdl->lcd_backlight.buf[0] == REGULATOR_MODE) {
		/*enable/disable backlight*/
		down(&disp_info->lcd_kit_sem);
		if (bl_level == 0 && last_bl_level != 0) {
			lcd_kit_charger_ctrl(LCD_KIT_BL, 0);
		} else if (last_bl_level == 0 && bl_level != 0) {
			lcd_kit_charger_ctrl(LCD_KIT_BL, 1);
		}
		last_bl_level = bl_level;
		up(&disp_info->lcd_kit_sem);
	}
	return ret;
}

int lcd_kit_mipi_set_backlight(struct hisi_fb_data_type* hisifd, uint32_t level)
{
	uint32_t bl_level = 0;
	static uint32_t last_bl_level = 255;
	uint32_t ret = LCD_KIT_OK;
	struct hisi_panel_info* pinfo = NULL;

	pinfo = &(hisifd->panel_info);
	bl_level = (level < hisifd->panel_info.bl_max) ? level : hisifd->panel_info.bl_max;
	hisifb_display_effect_fine_tune_backlight(hisifd, (int)bl_level, (int*)&bl_level);
	bl_flicker_detector_collect_device_bl(bl_level);
	ret = common_ops->set_mipi_backlight(hisifd, bl_level);
	if (power_hdl->lcd_backlight.buf[0] == REGULATOR_MODE) {
		/*enable/disable backlight*/
		down(&disp_info->lcd_kit_sem);
		if (bl_level == 0 && last_bl_level != 0) {
			lcd_kit_charger_ctrl(LCD_KIT_BL, 0);
		} else if (last_bl_level == 0 && bl_level != 0) {
			lcd_kit_charger_ctrl(LCD_KIT_BL, 1);
			LCD_KIT_INFO("bl_level = %d!\n", bl_level);
		}
		last_bl_level = bl_level;
		up(&disp_info->lcd_kit_sem);
	}
	return ret;
}

int lcd_kit_is_enter_sleep_mode(void)
{
	int sleep_mode = 0;

	if (common_info->pt.support) {
		sleep_mode = common_info->pt.mode;
	}
	return sleep_mode;
}

int lcd_kit_dsi_fifo_is_full(char __iomem* dsi_base)
{
	unsigned long dw_jiffies = 0;
	uint32_t pkg_status = 0;
	uint32_t phy_status = 0;
	int is_timeout = 1;

	/*read status register*/
	dw_jiffies = jiffies + HZ;
	do {
		pkg_status = inp32(dsi_base + MIPIDSI_CMD_PKT_STATUS_OFFSET);
		phy_status = inp32(dsi_base + MIPIDSI_PHY_STATUS_OFFSET);
		if ((pkg_status & 0x2) != 0x2 && !(phy_status & 0x2)) {
			is_timeout = 0;
			break;
		}
	} while (time_after(dw_jiffies, jiffies));

	if (is_timeout) {
		HISI_FB_ERR("mipi check empty fail: \n \
						MIPIDSI_CMD_PKT_STATUS = 0x%x \n \
						MIPIDSI_PHY_STATUS = 0x%x \n \
						MIPIDSI_INT_ST1_OFFSET = 0x%x \n",
					inp32(dsi_base + MIPIDSI_CMD_PKT_STATUS_OFFSET),
					inp32(dsi_base + MIPIDSI_PHY_STATUS_OFFSET),
					inp32(dsi_base + MIPIDSI_INT_ST1_OFFSET));
		return LCD_KIT_FAIL;
	}
	return LCD_KIT_OK;
}

int lcd_kit_dsi_fifo_is_empty(char __iomem* dsi_base)
{
    unsigned long dw_jiffies = 0;
    uint32_t pkg_status = 0;
    uint32_t phy_status = 0;
    int is_timeout = 1;

    /*read status register*/
    dw_jiffies = jiffies + HZ;
    do {
        pkg_status = inp32(dsi_base + MIPIDSI_CMD_PKT_STATUS_OFFSET);
        phy_status = inp32(dsi_base + MIPIDSI_PHY_STATUS_OFFSET);
        if ((pkg_status & 0x1) == 0x1 && !(phy_status & 0x2)){
            is_timeout = 0;
            break;
        }
    } while (time_after(dw_jiffies, jiffies));

    if (is_timeout) {
        HISI_FB_ERR("mipi check empty fail: \n \
            MIPIDSI_CMD_PKT_STATUS = 0x%x \n \
            MIPIDSI_PHY_STATUS = 0x%x \n \
            MIPIDSI_INT_ST1_OFFSET = 0x%x \n",
            inp32(dsi_base + MIPIDSI_CMD_PKT_STATUS_OFFSET),
            inp32(dsi_base + MIPIDSI_PHY_STATUS_OFFSET),
            inp32(dsi_base + MIPIDSI_INT_ST1_OFFSET));
        return LCD_KIT_FAIL;
    }
    return LCD_KIT_OK;
}

static void lcd_kit_vesa_para_parse(struct device_node* np, struct hisi_panel_info* pinfo)
{
	if (!pinfo) {
		LCD_KIT_ERR("pinfo is null\n");
		return ;
	}

	OF_PROPERTY_READ_U32_DEFAULT(np, "lcd-kit,vesa-slice-width", &pinfo->vesa_dsc.slice_width, 0);
	OF_PROPERTY_READ_U32_DEFAULT(np, "lcd-kit,vesa-slice-height", &pinfo->vesa_dsc.slice_height, 0);
}

void lcd_kit_pinfo_init(struct device_node* np, struct hisi_panel_info* pinfo)
{
	if (!pinfo) {
		LCD_KIT_ERR("pinfo is null\n");
		return ;
	}

	OF_PROPERTY_READ_U32_DEFAULT(np, "lcd-kit,panel-xres", &pinfo->xres, 1440);
	OF_PROPERTY_READ_U32_DEFAULT(np, "lcd-kit,panel-yres", &pinfo->yres, 2560);
	OF_PROPERTY_READ_U32_DEFAULT(np, "lcd-kit,panel-width", &pinfo->width, 73);
	OF_PROPERTY_READ_U32_DEFAULT(np, "lcd-kit,panel-height", &pinfo->height, 130);
	OF_PROPERTY_READ_U32_DEFAULT(np, "lcd-kit,panel-orientation", &pinfo->orientation, 0);
	OF_PROPERTY_READ_U32_DEFAULT(np, "lcd-kit,panel-bpp", &pinfo->bpp, 0);
	OF_PROPERTY_READ_U32_DEFAULT(np, "lcd-kit,panel-bgr-fmt", &pinfo->bgr_fmt, 0);
	OF_PROPERTY_READ_U32_DEFAULT(np, "lcd-kit,panel-bl-type", &pinfo->bl_set_type, 0);
	OF_PROPERTY_READ_U32_DEFAULT(np, "lcd-kit,panel-bl-min", &pinfo->bl_min, 0);
	OF_PROPERTY_READ_U32_DEFAULT(np, "lcd-kit,panel-bl-max", &pinfo->bl_max, 0);
	OF_PROPERTY_READ_U32_DEFAULT(np, "lcd-kit,panel-bl-def", &pinfo->bl_default, 0);
	OF_PROPERTY_READ_U32_DEFAULT(np, "lcd-kit,panel-cmd-type", &pinfo->type, 0);
	OF_PROPERTY_READ_U8_DEFAULT(np, "lcd-kit,panel-frc-enable", &pinfo->frc_enable, 0);
	OF_PROPERTY_READ_U8_DEFAULT(np, "lcd-kit,panel-esd-skip-mipi-check", &pinfo->esd_skip_mipi_check, 1);
	OF_PROPERTY_READ_U32_DEFAULT(np, "lcd-kit,panel-ifbc-type", &pinfo->ifbc_type, 0);
	OF_PROPERTY_READ_U32_DEFAULT(np, "lcd-kit,panel-bl-ic-ctrl-type", &pinfo->bl_ic_ctrl_mode, 0);
	OF_PROPERTY_READ_U32_DEFAULT(np, "lcd-kit,blpwm-div", &pinfo->blpwm_out_div_value, 0);
	OF_PROPERTY_READ_U64_RETURN(np, "lcd-kit,panel-pxl-clk", &pinfo->pxl_clk_rate);
	OF_PROPERTY_READ_U32_DEFAULT(np, "lcd-kit,panel-pxl-clk-div", &pinfo->pxl_clk_rate_div, 1);
	OF_PROPERTY_READ_U32_DEFAULT(np, "lcd-kit,panel-vsyn-ctr-type", &pinfo->vsync_ctrl_type, 0);
	OF_PROPERTY_READ_U32_DEFAULT(np, "lcd-kit,panel-xcc-set-in-isr-support", &pinfo->xcc_set_in_isr_support, 0);
	OF_PROPERTY_READ_U32_DEFAULT(np, "lcd-kit,panel-bl-pwm-preci-type", &pinfo->blpwm_precision_type, 0);
	OF_PROPERTY_READ_U8_DEFAULT(np, "lcd-kit,non-check-ldi-porch", &pinfo->non_check_ldi_porch, 0);

	/*effect info*/
	OF_PROPERTY_READ_U8_DEFAULT(np, "lcd-kit,sbl-support", &pinfo->sbl_support, 0);
	OF_PROPERTY_READ_U8_DEFAULT(np, "lcd-kit,gamma-support", &pinfo->gamma_support, 0);
	OF_PROPERTY_READ_U8_DEFAULT(np, "lcd-kit,gmp-support", &pinfo->gmp_support, 0);
	OF_PROPERTY_READ_U8_DEFAULT(np, "lcd-kit,dbv-curve-mapped-support", &pinfo->dbv_curve_mapped_support, 0);
	OF_PROPERTY_READ_U8_DEFAULT(np, "lcd-kit,color-temp-support", &pinfo->color_temperature_support, 0);
	OF_PROPERTY_READ_U8_DEFAULT(np, "lcd-kit,color-temp-rectify-support", &pinfo->color_temp_rectify_support, 0);
	OF_PROPERTY_READ_U8_DEFAULT(np, "lcd-kit,comform-mode-support", &pinfo->comform_mode_support, 0);
	OF_PROPERTY_READ_U8_DEFAULT(np, "lcd-kit,cinema-mode-support", &pinfo->cinema_mode_support, 0);
	OF_PROPERTY_READ_U8_DEFAULT(np, "lcd-kit,xcc-support", &pinfo->xcc_support, 0);
	OF_PROPERTY_READ_U8_DEFAULT(np, "lcd-kit,hiace-support", &pinfo->hiace_support, 0);
	OF_PROPERTY_READ_U8_DEFAULT(np, "lcd-kit,panel-ce-support", &pinfo->panel_effect_support, 0);
	OF_PROPERTY_READ_U8_DEFAULT(np, "lcd-kit,arsr1p-sharpness-support", &pinfo->arsr1p_sharpness_support, 0);
	OF_PROPERTY_READ_U8_DEFAULT(np, "lcd-kit,prefix-sharp-1D-support", &pinfo->prefix_sharpness1D_support, 0);
	OF_PROPERTY_READ_U8_DEFAULT(np, "lcd-kit,prefix-sharp-2D-support", &pinfo->prefix_sharpness2D_support, 0);
	if (pinfo->hiace_support) {
		OF_PROPERTY_READ_U32_DEFAULT(np, "lcd-kit,iglobal-hist-black-pos", &pinfo->hiace_param.iGlobalHistBlackPos, 0);
		OF_PROPERTY_READ_U32_DEFAULT(np, "lcd-kit,iglobal-hist-white-pos", &pinfo->hiace_param.iGlobalHistWhitePos, 0);
		OF_PROPERTY_READ_U32_DEFAULT(np, "lcd-kit,iglobal-hist-black-weight", &pinfo->hiace_param.iGlobalHistBlackWeight, 0);
		OF_PROPERTY_READ_U32_DEFAULT(np, "lcd-kit,iglobal-hist-white-weight", &pinfo->hiace_param.iGlobalHistWhiteWeight, 0);
		OF_PROPERTY_READ_U32_DEFAULT(np, "lcd-kit,iglobal-hist-zero-cut-ratio", &pinfo->hiace_param.iGlobalHistZeroCutRatio, 0);
		OF_PROPERTY_READ_U32_DEFAULT(np, "lcd-kit,iglobal-hist-slope-cut-ratio", &pinfo->hiace_param.iGlobalHistSlopeCutRatio, 0);
		OF_PROPERTY_READ_U32_DEFAULT(np, "lcd-kit,imax-lcd-luminance", &pinfo->hiace_param.iMaxLcdLuminance, 0);
		OF_PROPERTY_READ_U32_DEFAULT(np, "lcd-kit,imin-lcd-luminance", &pinfo->hiace_param.iMinLcdLuminance, 0);
	}
	OF_PROPERTY_READ_U32_RETURN(np, "lcd-kit,color-temp-rectify-r", &pinfo->color_temp_rectify_R);
	OF_PROPERTY_READ_U32_RETURN(np, "lcd-kit,color-temp-rectify-g", &pinfo->color_temp_rectify_G);
	OF_PROPERTY_READ_U32_RETURN(np, "lcd-kit,color-temp-rectify-b", &pinfo->color_temp_rectify_B);
	OF_PROPERTY_READ_U8_RETURN(np, "lcd-kit,prefix-ce-support", &pinfo->prefix_ce_support);
	OF_PROPERTY_READ_U8_RETURN(np, "lcd-kit,acm-support", &pinfo->acm_support);
	OF_PROPERTY_READ_U8_RETURN(np, "lcd-kit,acm-ce-support", &pinfo->acm_ce_support);
	OF_PROPERTY_READ_U8_RETURN(np, "lcd-kit,smart-color-mode-support", &pinfo->smart_color_mode_support);
	OF_PROPERTY_READ_U32_RETURN(np, "lcd-kit,acm-valid-num", &pinfo->acm_valid_num);
	OF_PROPERTY_READ_U32_RETURN(np, "lcd-kit,r_hh0", &pinfo->r0_hh);
	OF_PROPERTY_READ_U32_RETURN(np, "lcd-kit,r_lh0", &pinfo->r0_lh);
	OF_PROPERTY_READ_U32_RETURN(np, "lcd-kit,r_hh1", &pinfo->r1_hh);
	OF_PROPERTY_READ_U32_RETURN(np, "lcd-kit,r_lh1", &pinfo->r1_lh);
	OF_PROPERTY_READ_U32_RETURN(np, "lcd-kit,r_hh2", &pinfo->r2_hh);
	OF_PROPERTY_READ_U32_RETURN(np, "lcd-kit,r_lh2", &pinfo->r2_lh);
	OF_PROPERTY_READ_U32_RETURN(np, "lcd-kit,r_hh3", &pinfo->r3_hh);
	OF_PROPERTY_READ_U32_RETURN(np, "lcd-kit,r_lh3", &pinfo->r3_lh);
	OF_PROPERTY_READ_U32_RETURN(np, "lcd-kit,r_hh4", &pinfo->r4_hh);
	OF_PROPERTY_READ_U32_RETURN(np, "lcd-kit,r_lh4", &pinfo->r4_lh);
	OF_PROPERTY_READ_U32_RETURN(np, "lcd-kit,r_hh5", &pinfo->r5_hh);
	OF_PROPERTY_READ_U32_RETURN(np, "lcd-kit,r_lh5", &pinfo->r5_lh);
	OF_PROPERTY_READ_U32_RETURN(np, "lcd-kit,r_hh6", &pinfo->r6_hh);
	OF_PROPERTY_READ_U32_RETURN(np, "lcd-kit,r_lh6", &pinfo->r6_lh);
	OF_PROPERTY_READ_U32_RETURN(np, "lcd-kit,r_video_hh0", &pinfo->video_r0_hh);
	OF_PROPERTY_READ_U32_RETURN(np, "lcd-kit,r_video_lh0", &pinfo->video_r0_lh);
	OF_PROPERTY_READ_U32_RETURN(np, "lcd-kit,r_video_hh1", &pinfo->video_r1_hh);
	OF_PROPERTY_READ_U32_RETURN(np, "lcd-kit,r_video_lh1", &pinfo->video_r1_lh);
	OF_PROPERTY_READ_U32_RETURN(np, "lcd-kit,r_video_hh2", &pinfo->video_r2_hh);
	OF_PROPERTY_READ_U32_RETURN(np, "lcd-kit,r_video_lh2", &pinfo->video_r2_lh);
	OF_PROPERTY_READ_U32_RETURN(np, "lcd-kit,r_video_hh3", &pinfo->video_r3_hh);
	OF_PROPERTY_READ_U32_RETURN(np, "lcd-kit,r_video_lh3", &pinfo->video_r3_lh);
	OF_PROPERTY_READ_U32_RETURN(np, "lcd-kit,r_video_hh4", &pinfo->video_r4_hh);
	OF_PROPERTY_READ_U32_RETURN(np, "lcd-kit,r_video_lh4", &pinfo->video_r4_lh);
	OF_PROPERTY_READ_U32_RETURN(np, "lcd-kit,r_video_hh5", &pinfo->video_r5_hh);
	OF_PROPERTY_READ_U32_RETURN(np, "lcd-kit,r_video_lh5", &pinfo->video_r5_lh);
	OF_PROPERTY_READ_U32_RETURN(np, "lcd-kit,r_video_hh6", &pinfo->video_r6_hh);
	OF_PROPERTY_READ_U32_RETURN(np, "lcd-kit,r_video_lh6", &pinfo->video_r6_lh);
	OF_PROPERTY_READ_U32_RETURN(np, "lcd-kit,mask-delay-time-before-fp", &pinfo->mask_delay_time_before_fp);
	OF_PROPERTY_READ_U32_RETURN(np, "lcd-kit,mask-delay-time-after-fp", &pinfo->mask_delay_time_after_fp);
	OF_PROPERTY_READ_U32_RETURN(np, "lcd-kit,bl-delay-frame", &pinfo->bl_delay_frame);

	/*sbl info*/
	OF_PROPERTY_READ_U32_RETURN(np, "lcd-kit,sbl-stren-limit", &pinfo->smart_bl.strength_limit);
	OF_PROPERTY_READ_U32_RETURN(np, "lcd-kit,sbl-cal-a", &pinfo->smart_bl.calibration_a);
	OF_PROPERTY_READ_U32_RETURN(np, "lcd-kit,sbl-cal-b", &pinfo->smart_bl.calibration_b);
	OF_PROPERTY_READ_U32_RETURN(np, "lcd-kit,sbl-cal-c", &pinfo->smart_bl.calibration_c);
	OF_PROPERTY_READ_U32_RETURN(np, "lcd-kit,sbl-cal-d", &pinfo->smart_bl.calibration_d);
	OF_PROPERTY_READ_U32_RETURN(np, "lcd-kit,sbl-tf-ctl", &pinfo->smart_bl.t_filter_control);
	OF_PROPERTY_READ_U32_RETURN(np, "lcd-kit,sbl-bl-min", &pinfo->smart_bl.backlight_min);
	OF_PROPERTY_READ_U32_RETURN(np, "lcd-kit,sbl-bl-max", &pinfo->smart_bl.backlight_max);
	OF_PROPERTY_READ_U32_RETURN(np, "lcd-kit,sbl-bl-scale", &pinfo->smart_bl.backlight_scale);
	OF_PROPERTY_READ_U32_RETURN(np, "lcd-kit,sbl-am-light-min", &pinfo->smart_bl.ambient_light_min);
	OF_PROPERTY_READ_U32_RETURN(np, "lcd-kit,sbl-filter-a", &pinfo->smart_bl.filter_a);
	OF_PROPERTY_READ_U32_RETURN(np, "lcd-kit,sbl-filter-b", &pinfo->smart_bl.filter_b);
	OF_PROPERTY_READ_U32_RETURN(np, "lcd-kit,sbl-logo-left", &pinfo->smart_bl.logo_left);
	OF_PROPERTY_READ_U32_RETURN(np, "lcd-kit,sbl-logo-top", &pinfo->smart_bl.logo_top);
	OF_PROPERTY_READ_U32_RETURN(np, "lcd-kit,sbl-variance-intensity-space", &pinfo->smart_bl.variance_intensity_space);
	OF_PROPERTY_READ_U32_RETURN(np, "lcd-kit,sbl-slope-max", &pinfo->smart_bl.slope_max);
	OF_PROPERTY_READ_U32_RETURN(np, "lcd-kit,sbl-slope-min", &pinfo->smart_bl.slope_min);

	OF_PROPERTY_READ_U32_RETURN(np, "lcd-kit,dpi01-set-change", &pinfo->dpi01_exchange_flag);
	/*ldi info*/
	OF_PROPERTY_READ_U32_RETURN(np, "lcd-kit,h-back-porch", &pinfo->ldi.h_back_porch);
	OF_PROPERTY_READ_U32_RETURN(np, "lcd-kit,h-front-porch", &pinfo->ldi.h_front_porch);
	OF_PROPERTY_READ_U32_RETURN(np, "lcd-kit,h-pulse-width", &pinfo->ldi.h_pulse_width);
	OF_PROPERTY_READ_U32_RETURN(np, "lcd-kit,v-back-porch", &pinfo->ldi.v_back_porch);
	OF_PROPERTY_READ_U32_RETURN(np, "lcd-kit,v-front-porch", &pinfo->ldi.v_front_porch);
	OF_PROPERTY_READ_U32_RETURN(np, "lcd-kit,v-pulse-width", &pinfo->ldi.v_pulse_width);
	OF_PROPERTY_READ_U8_RETURN(np, "lcd-kit,ldi-hsync-plr", &pinfo->ldi.hsync_plr);
	OF_PROPERTY_READ_U8_RETURN(np, "lcd-kit,ldi-vsync-plr", &pinfo->ldi.vsync_plr);
	OF_PROPERTY_READ_U8_RETURN(np, "lcd-kit,ldi-pixel-clk-plr", &pinfo->ldi.pixelclk_plr);
	OF_PROPERTY_READ_U8_RETURN(np, "lcd-kit,ldi-data-en-plr", &pinfo->ldi.data_en_plr);
	OF_PROPERTY_READ_U8_DEFAULT(np, "lcd-kit,ldi-dpi0-overlap-size", &pinfo->ldi.dpi0_overlap_size, 0);
	OF_PROPERTY_READ_U8_DEFAULT(np, "lcd-kit,ldi-dpi1-overlap-size", &pinfo->ldi.dpi1_overlap_size, 0);

	/*mipi info*/
	OF_PROPERTY_READ_U8_DEFAULT(np, "lcd-kit,mipi-lane-nums", &pinfo->mipi.lane_nums, 0);
	OF_PROPERTY_READ_U8_DEFAULT(np, "lcd-kit,mipi-color-mode", &pinfo->mipi.color_mode, 0);
	OF_PROPERTY_READ_U8_DEFAULT(np, "lcd-kit,mipi-vc", &pinfo->mipi.vc, 0);
	OF_PROPERTY_READ_U32_DEFAULT(np, "lcd-kit,mipi-burst-mode", &pinfo->mipi.burst_mode, 0);
	OF_PROPERTY_READ_U32_DEFAULT(np, "lcd-kit,mipi-dsi-bit-clk", &pinfo->mipi.dsi_bit_clk, 0);
	OF_PROPERTY_READ_U32_DEFAULT(np, "lcd-kit,mipi-max-tx-esc-clk", &pinfo->mipi.max_tx_esc_clk, 0);
	OF_PROPERTY_READ_U32_DEFAULT(np, "lcd-kit,mipi-dsi-bit-clk-val1", &pinfo->mipi.dsi_bit_clk_val1, 0);
	OF_PROPERTY_READ_U32_DEFAULT(np, "lcd-kit,mipi-dsi-bit-clk-val2", &pinfo->mipi.dsi_bit_clk_val2, 0);
	OF_PROPERTY_READ_U32_DEFAULT(np, "lcd-kit,mipi-dsi-bit-clk-val3", &pinfo->mipi.dsi_bit_clk_val3, 0);
	OF_PROPERTY_READ_U32_DEFAULT(np, "lcd-kit,mipi-dsi-bit-clk-val4", &pinfo->mipi.dsi_bit_clk_val4, 0);
	OF_PROPERTY_READ_U32_DEFAULT(np, "lcd-kit,mipi-dsi-bit-clk-val5", &pinfo->mipi.dsi_bit_clk_val5, 0);
	OF_PROPERTY_READ_U8_DEFAULT(np, "lcd-kit,mipi-non-continue-enable", &pinfo->mipi.non_continue_en, 0);
	OF_PROPERTY_READ_U32_DEFAULT(np, "lcd-kit,mipi-clk-post-adjust", &pinfo->mipi.clk_post_adjust, 0);
	OF_PROPERTY_READ_U32_DEFAULT(np, "lcd-kit,mipi-clk-pre-adjust", &pinfo->mipi.clk_pre_adjust, 0);
	OF_PROPERTY_READ_U32_DEFAULT(np, "lcd-kit,mipi-clk-t-hs-prepare-adjust", &pinfo->mipi.clk_t_hs_prepare_adjust, 0);
	OF_PROPERTY_READ_U32_DEFAULT(np, "lcd-kit,mipi-clk-t-lpx-adjust", &pinfo->mipi.clk_t_lpx_adjust, 0);
	OF_PROPERTY_READ_U32_DEFAULT(np, "lcd-kit,mipi-data-t-lpx-adjust", &pinfo->mipi.data_t_lpx_adjust, 0);
	OF_PROPERTY_READ_U32_DEFAULT(np, "lcd-kit,mipi-clk-t-hs-trail-adjust", &pinfo->mipi.clk_t_hs_trial_adjust, 0);
	OF_PROPERTY_READ_U32_DEFAULT(np, "lcd-kit,mipi-clk-t-hs-exit-adjust", &pinfo->mipi.clk_t_hs_exit_adjust, 0);
	OF_PROPERTY_READ_U32_DEFAULT(np, "lcd-kit,mipi-clk-t-hs-zero-adjust", &pinfo->mipi.clk_t_hs_zero_adjust, 0);
	OF_PROPERTY_READ_U32_DEFAULT(np, "lcd-kit,mipi-data-t-hs-zero-adjust", &pinfo->mipi.data_t_hs_zero_adjust, 0);
	OF_PROPERTY_READ_U32_DEFAULT(np, "lcd-kit,mipi-data-t-hs-trail-adjust", &pinfo->mipi.data_t_hs_trial_adjust, 0);
	OF_PROPERTY_READ_U32_DEFAULT(np, "lcd-kit,mipi-rg-vrefsel-vcm-adjust", &pinfo->mipi.rg_vrefsel_vcm_adjust, 0);
	OF_PROPERTY_READ_U32_DEFAULT(np, "lcd-kit,mipi-phy-mode", &pinfo->mipi.phy_mode, 0);
	OF_PROPERTY_READ_U32_DEFAULT(np, "lcd-kit,mipi-lp11-flag", &pinfo->mipi.lp11_flag, 0);
	OF_PROPERTY_READ_U32_DEFAULT(np, "lcd-kit,mipi-hs-wr-to-time", &pinfo->mipi.hs_wr_to_time, 0);
	OF_PROPERTY_READ_U32_DEFAULT(np, "lcd-kit,mipi-phy-update", &pinfo->mipi.phy_m_n_count_update, 0);
	OF_PROPERTY_READ_U8_DEFAULT(np, "lcd-kit,mipi-dsi-upt-support", &pinfo->dsi_bit_clk_upt_support, 0);

	/*dirty region update*/
	if (common_info->dirty_region.support) {
		pinfo->dirty_region_updt_support = common_info->dirty_region.support;
		OF_PROPERTY_READ_DIRTYREGION_INFO_RETURN(np, "lcd-kit,dirty-left-align", &pinfo->dirty_region_info.left_align);
		OF_PROPERTY_READ_DIRTYREGION_INFO_RETURN(np, "lcd-kit,dirty-right-align", &pinfo->dirty_region_info.right_align);
		OF_PROPERTY_READ_DIRTYREGION_INFO_RETURN(np, "lcd-kit,dirty-top-align", &pinfo->dirty_region_info.top_align);
		OF_PROPERTY_READ_DIRTYREGION_INFO_RETURN(np, "lcd-kit,dirty-bott-align", &pinfo->dirty_region_info.bottom_align);
		OF_PROPERTY_READ_DIRTYREGION_INFO_RETURN(np, "lcd-kit,dirty-width-align", &pinfo->dirty_region_info.w_align);
		OF_PROPERTY_READ_DIRTYREGION_INFO_RETURN(np, "lcd-kit,dirty-height-align", &pinfo->dirty_region_info.h_align);
		OF_PROPERTY_READ_DIRTYREGION_INFO_RETURN(np, "lcd-kit,dirty-width-min", &pinfo->dirty_region_info.w_min);
		OF_PROPERTY_READ_DIRTYREGION_INFO_RETURN(np, "lcd-kit,dirty-height-min", &pinfo->dirty_region_info.h_min);
		OF_PROPERTY_READ_DIRTYREGION_INFO_RETURN(np, "lcd-kit,dirty-top-start", &pinfo->dirty_region_info.top_start);
		OF_PROPERTY_READ_DIRTYREGION_INFO_RETURN(np, "lcd-kit,dirty-bott-start", &pinfo->dirty_region_info.bottom_start);
	}
	/*bl pwm*/
	OF_PROPERTY_READ_U32_DEFAULT(np, "lcd-kit,blpwm-disable", &pinfo->blpwm_input_disable, 0);
	if (pinfo->bl_set_type==BL_SET_BY_BLPWM && pinfo->blpwm_input_disable==0) {
		pinfo->blpwm_input_ena = 1;
	}
	if (common_info->esd.support) {
		pinfo->esd_enable = 1;
	}

	pinfo->rgbw_support = disp_info->rgbw.support;
	pinfo->lcd_uninit_step_support = 1;
	pinfo->pxl_clk_rate = pinfo->pxl_clk_rate * 1000000UL;
	pinfo->mipi.dsi_bit_clk_upt = pinfo->mipi.dsi_bit_clk;
	pinfo->mipi.max_tx_esc_clk = pinfo->mipi.max_tx_esc_clk * 1000000; 
	pinfo->panel_name = common_info->panel_name;
	return ;
}

int lcd_kit_panel_version_init(struct hisi_fb_data_type* hisifd)
{
	int i = 0;
	int j = 0;
	int ret = LCD_KIT_OK;

	if (hisifd == NULL) {
		LCD_KIT_ERR("hisifd is null\n");
		return LCD_KIT_FAIL;
	}

	ret = lcd_kit_dsi_cmds_rx(hisifd, (uint8_t*)disp_info->panel_version.read_value, &disp_info->panel_version.cmds);
	if (ret) {
		LCD_KIT_ERR("cmd fail\n");
		return LCD_KIT_FAIL;
	}

	for (i = 0; i < (int)disp_info->panel_version.version_number; i++) {
		for (j = 0; j < (int)disp_info->panel_version.value_number; j++) {
			LCD_KIT_INFO("disp_info->panel_version.read_value[%d]:0x%x  \n", j, disp_info->panel_version.read_value[j]);
			LCD_KIT_INFO("disp_info->panel_version.value.arry_data[%d].buf[%d]:0x%x \n",  i, j, disp_info->panel_version.value.arry_data[i].buf[j]);
			if (disp_info->panel_version.read_value[j] != disp_info->panel_version.value.arry_data[i].buf[j]) {
				break;
			}

			if (j == ((int)disp_info->panel_version.value_number - 1)) {
				memcpy(hisifd->panel_info.lcd_panel_version, " VER:", strlen(" VER:") + 1);
				strncat (hisifd->panel_info.lcd_panel_version,disp_info->panel_version.lcd_version_name[i],strlen(disp_info->panel_version.lcd_version_name[i]));
				LCD_KIT_INFO("Panel version is %s\n", hisifd->panel_info.lcd_panel_version);
				return LCD_KIT_OK;
			}
		}
	}

	if (i == disp_info->panel_version.version_number) {
		LCD_KIT_INFO("panel_version not find \n");
		return LCD_KIT_FAIL;
	}

	return LCD_KIT_FAIL;
}


void lcd_kit_factory_init(struct hisi_panel_info* pinfo)
{
	if (runmode_is_factory()) {
		pinfo->esd_enable = 0;
		pinfo->dirty_region_updt_support = 0;
		pinfo->prefix_ce_support = 0;
		pinfo->prefix_sharpness1D_support = 0;
		pinfo->prefix_sharpness2D_support = 0;
		pinfo->sbl_support = 0;
		pinfo->acm_support = 0;
		pinfo->acm_ce_support = 0;
		pinfo->esd_enable = 0;
		pinfo->comform_mode_support = 0;
		pinfo->color_temp_rectify_support = 0;
		pinfo->hiace_support = 0;
		pinfo->arsr1p_sharpness_support = 0;
		pinfo->blpwm_input_ena = 0;
		pinfo->gmp_support = 0;
		pinfo->vsync_ctrl_type = 0;
		common_info->effect_on.support = 0;
		common_info->effect_color.mode &= BITS(31);
		disp_info->fps.support = 0;
	}
}

void lcd_kit_parse_running(struct device_node* np)
{
	int ret = LCD_KIT_OK;
	char *name[LDO_NUM_MAX];
	int i = 0;

	/*pcd errflag check*/
	OF_PROPERTY_READ_U32_DEFAULT(np, "lcd-kit,pcd-errflag-check-support", &disp_info->pcd_errflag_check_support, 0);
	OF_PROPERTY_READ_U32_DEFAULT(np, "lcd-kit,gpio-pcd", &disp_info->gpio_pcd, 0);
	OF_PROPERTY_READ_U32_DEFAULT(np, "lcd-kit,gpio-errflag", &disp_info->gpio_errflag, 0);
	/*backlight open short test*/
	OF_PROPERTY_READ_U32_DEFAULT(np, "lcd-kit,bkl-open-short-support", &disp_info->bkl_open_short_support, 0);
	/*check sum*/
	OF_PROPERTY_READ_U32_DEFAULT(np, "lcd-kit,checksum-support", &disp_info->checksum.support, 0);
	if (disp_info->checksum.support) {
		lcd_kit_parse_dcs_cmds(np, "lcd-kit,checksum-enable-cmds", "lcd-kit,checksum-enable-cmds-state",
							   &disp_info->checksum.enable_cmds);
		lcd_kit_parse_dcs_cmds(np, "lcd-kit,checksum-disable-cmds", "lcd-kit,checksum-disable-cmds-state",
							   &disp_info->checksum.disable_cmds);
		lcd_kit_parse_dcs_cmds(np, "lcd-kit,checksum-cmds", "lcd-kit,checksum-cmds-state",
							   &disp_info->checksum.checksum_cmds);
		lcd_kit_parse_array_data(np, "lcd-kit,checksum-value", &disp_info->checksum.value);
	}
	/*hkadc*/
	OF_PROPERTY_READ_U32_DEFAULT(np, "lcd-kit,hkadc-support", &disp_info->hkadc.support, 0);
	if (disp_info->hkadc.support) {
		OF_PROPERTY_READ_U32_DEFAULT(np, "lcd-kit,hkadc-value", &disp_info->hkadc.value, 0);
	}
	/*current detect*/
	OF_PROPERTY_READ_U32_DEFAULT(np, "lcd-kit,current-det-support", &disp_info->current_det.support, 0);
	if (disp_info->current_det.support) {
		lcd_kit_parse_dcs_cmds(np, "lcd-kit,current-det-cmds", "lcd-kit,current-det-cmds-state",
								&disp_info->current_det.detect_cmds);
		lcd_kit_parse_array_data(np, "lcd-kit,current-det-value", &disp_info->current_det.value);
	}
	/*low voltage detect*/
	OF_PROPERTY_READ_U32_DEFAULT(np, "lcd-kit,lv-det-support", &disp_info->lv_det.support, 0);
	if (disp_info->lv_det.support) {
		lcd_kit_parse_dcs_cmds(np, "lcd-kit,lv-det-cmds", "lcd-kit,lv-det-cmds-state",
								&disp_info->lv_det.detect_cmds);
		lcd_kit_parse_array_data(np, "lcd-kit,lv-det-value", &disp_info->lv_det.value);
	}
	/*ldo check*/
	OF_PROPERTY_READ_U32_DEFAULT(np, "lcd-kit,ldo-check-support", &disp_info->ldo_check.support, 0);
	if (disp_info->ldo_check.support) {
		disp_info->ldo_check.ldo_num = of_property_count_elems_of_size(np, "lcd-kit,ldo-check-channel", sizeof(u32));
		if (disp_info->ldo_check.ldo_num > 0) {
			ret = of_property_read_u32_array(np, "lcd-kit,ldo-check-channel",
								disp_info->ldo_check.ldo_channel, disp_info->ldo_check.ldo_num);
			if (ret < 0) {
				LCD_KIT_ERR("parse ldo channel fail\n");
			}
			ret = of_property_read_u32_array(np, "lcd-kit,ldo-check-threshold",
								disp_info->ldo_check.curr_threshold, disp_info->ldo_check.ldo_num);
			if (ret < 0) {
				LCD_KIT_ERR("parse current threshold fail\n");
			}
			ret = of_property_read_string_array(np, "lcd-kit,ldo-check-name",
								(const char **)&name[0], disp_info->ldo_check.ldo_num);
			if (ret < 0) {
				LCD_KIT_ERR("parse ldo name fail\n");
			}
			for (i = 0; i < (int)disp_info->ldo_check.ldo_num; i++) {
				strncpy(disp_info->ldo_check.ldo_name[i], name[i], LDO_NAME_LEN_MAX - 1);
			}
		}
	}
	return ;
}

void lcd_kit_parse_effect(struct device_node* np)
{
	/*gamma calibration*/
	OF_PROPERTY_READ_U32_DEFAULT(np, "lcd-kit,gamma-cal-support", &disp_info->gamma_cal.support, 0);
	if (disp_info->gamma_cal.support) {
		lcd_kit_parse_dcs_cmds(np, "lcd-kit,gamma-cal-cmds", "lcd-kit,gamma-cal-cmds-state",
							   &disp_info->gamma_cal.cmds);
	}
	/*brightness and color uniform*/
	OF_PROPERTY_READ_U32_DEFAULT(np, "lcd-kit,brightness-color-uniform-support", &disp_info->oeminfo.brightness_color_uniform.support, 0);
	if (disp_info->oeminfo.brightness_color_uniform.support) {
		lcd_kit_parse_dcs_cmds(np, "lcd-kit,brightness-color-cmds", "lcd-kit,brightness-color-cmds-state",
							   &disp_info->oeminfo.brightness_color_uniform.brightness_color_cmds);
	}
	/*oem information*/
	OF_PROPERTY_READ_U32_DEFAULT(np, "lcd-kit,oem-info-support", &disp_info->oeminfo.support, 0);
	if (disp_info->oeminfo.support) {
		OF_PROPERTY_READ_U32_DEFAULT(np, "lcd-kit,oem-barcode-2d-support", &disp_info->oeminfo.barcode_2d.support, 0);
		if (disp_info->oeminfo.barcode_2d.support) {
			lcd_kit_parse_dcs_cmds(np, "lcd-kit,barcode-2d-cmds", "lcd-kit,barcode-2d-cmds-state",
									&disp_info->oeminfo.barcode_2d.cmds);
		}
	}
	/*rgbw*/
	OF_PROPERTY_READ_U32_DEFAULT(np, "lcd-kit,rgbw-support", &disp_info->rgbw.support, 0);
	if (disp_info->rgbw.support) {
		OF_PROPERTY_READ_U32_DEFAULT(np, "lcd-kit,rgbw-bl-max", &disp_info->rgbw.rgbw_bl_max, 0);
		lcd_kit_parse_dcs_cmds(np, "lcd-kit,rgbw-mode1-cmds", "lcd-kit,rgbw-mode1-cmds-state",
								&disp_info->rgbw.mode1_cmds);
		lcd_kit_parse_dcs_cmds(np, "lcd-kit,rgbw-mode2-cmds", "lcd-kit,rgbw-mode2-cmds-state",
								&disp_info->rgbw.mode2_cmds);
		lcd_kit_parse_dcs_cmds(np, "lcd-kit,rgbw-mode3-cmds", "lcd-kit,rgbw-mode3-cmds-state",
								&disp_info->rgbw.mode3_cmds);
		lcd_kit_parse_dcs_cmds(np, "lcd-kit,rgbw-mode4-cmds", "lcd-kit,rgbw-mode4-cmds-state",
								&disp_info->rgbw.mode4_cmds);
		lcd_kit_parse_dcs_cmds(np, "lcd-kit,rgbw-backlight-cmds", "lcd-kit,rgbw-backlight-cmds-state",
								&disp_info->rgbw.backlight_cmds);
		lcd_kit_parse_dcs_cmds(np, "lcd-kit,rgbw-saturation-cmds", "lcd-kit,rgbw-saturation-cmds-state",
								&disp_info->rgbw.saturation_ctrl_cmds);
		lcd_kit_parse_dcs_cmds(np, "lcd-kit,rgbw-frame-gain-limit-cmds", "lcd-kit,rgbw-frame-gain-limit-cmds-state",
								&disp_info->rgbw.frame_gain_limit_cmds);
		lcd_kit_parse_dcs_cmds(np, "lcd-kit,rgbw-frame-gain-speed-cmds", "lcd-kit,rgbw-frame-gain-speed-cmds-state",
								&disp_info->rgbw.frame_gain_speed_cmds);
		lcd_kit_parse_dcs_cmds(np, "lcd-kit,rgbw-color-distor-allowance-cmds", "lcd-kit,rgbw-color-distor-allowance-cmds-state",
								&disp_info->rgbw.color_distor_allowance_cmds);
		lcd_kit_parse_dcs_cmds(np, "lcd-kit,rgbw-pixel-gain-limit-cmds", "lcd-kit,rgbw-pixel-gain-limit-cmds-state",
								&disp_info->rgbw.pixel_gain_limit_cmds);
		lcd_kit_parse_dcs_cmds(np, "lcd-kit,rgbw-pixel-gain-speed-cmds", "lcd-kit,rgbw-pixel-gain-speed-cmds-state",
								&disp_info->rgbw.pixel_gain_speed_cmds);
		lcd_kit_parse_dcs_cmds(np, "lcd-kit,rgbw-pwm-gain-cmds", "lcd-kit,rgbw-pwm-gain-cmds-state",
								&disp_info->rgbw.pwm_gain_cmds);
	}
	return ;
}
/*lint -save -e* */
void lcd_kit_parse_util(struct device_node* np)
{
	char *name[VERSION_NUM_MAX];
	int i = 0;
	int ret = LCD_KIT_OK;

	/*alpm*/
	OF_PROPERTY_READ_U32_DEFAULT(np, "lcd-kit,alpm-support", &disp_info->alpm.support, 0);
	if (disp_info->alpm.support) {
		lcd_kit_parse_dcs_cmds(np, "lcd-kit,alpm-low-light-cmds", "lcd-kit,alpm-low-light-cmds-state",
							   &disp_info->alpm.low_light_cmds);
		lcd_kit_parse_dcs_cmds(np, "lcd-kit,alpm-off-cmds", "lcd-kit,alpm-off-cmds-state",
							   &disp_info->alpm.off_cmds);
		lcd_kit_parse_dcs_cmds(np, "lcd-kit,alpm-exit-cmds", "lcd-kit,alpm-exit-cmds-state",
							   &disp_info->alpm.exit_cmds);
		lcd_kit_parse_dcs_cmds(np, "lcd-kit,alpm-high-light-cmds", "lcd-kit,alpm-high-light-cmds-state",
							   &disp_info->alpm.high_light_cmds);
	}
	/*quickly sleep out*/
	OF_PROPERTY_READ_U32_DEFAULT(np, "lcd-kit,quickly-sleep-out-support", &disp_info->quickly_sleep_out.support, 0);
	if (disp_info->quickly_sleep_out.support) {
		OF_PROPERTY_READ_U32_DEFAULT(np, "lcd-kit,quickly-sleep-out-interval", &disp_info->quickly_sleep_out.interval, 0);
	}
	/*fps*/
	OF_PROPERTY_READ_U32_DEFAULT(np, "lcd-kit,fps-support", &disp_info->fps.support, 0);
	if (disp_info->fps.support) {
		lcd_kit_parse_dcs_cmds(np, "lcd-kit,fps-dfr-disable-cmds", "lcd-kit,fps-dfr-disable-cmds-state",
							   &disp_info->fps.dfr_disable_cmds);
		lcd_kit_parse_dcs_cmds(np, "lcd-kit,fps-dfr-enable-cmds", "lcd-kit,fps-dfr-enable-cmds-state",
							   &disp_info->fps.dfr_enable_cmds);
		lcd_kit_parse_dcs_cmds(np, "lcd-kit,fps-to-30-cmds", "lcd-kit,fps-to-30-cmds-state",
							   &disp_info->fps.fps_to_30_cmds);
		lcd_kit_parse_dcs_cmds(np, "lcd-kit,fps-to-60-cmds", "lcd-kit,fps-to-60-cmds-state",
							   &disp_info->fps.fps_to_60_cmds);
	}
	/*project id*/
	OF_PROPERTY_READ_U32_DEFAULT(np, "lcd-kit,project-id-support", &disp_info->project_id.support, 0);
	if (disp_info->project_id.support) {
		lcd_kit_parse_dcs_cmds(np, "lcd-kit,project-id-cmds", "lcd-kit,project-id-cmds-state",
							   &disp_info->project_id.cmds);
		disp_info->project_id.default_project_id = (char*)of_get_property(np, "lcd-kit,default-project-id", NULL);
	}
	OF_PROPERTY_READ_U32_DEFAULT(np, "lcd-kit,dsi1-support", &disp_info->dsi1_cmd_support, 0);

	/*panel version*/
	OF_PROPERTY_READ_U32_DEFAULT(np, "lcd-kit,panel-version-support", &disp_info->panel_version.support, 0);
	if (disp_info->panel_version.support) {
		lcd_kit_parse_dcs_cmds(np, "lcd-kit,panel-version-cmds", "lcd-kit,panel-version-cmds-state", &disp_info->panel_version.cmds);
		disp_info->panel_version.value_number = disp_info->panel_version.cmds.cmds->dlen - disp_info->panel_version.cmds.cmds->payload[1];
		lcd_kit_parse_arrays_data(np, "lcd-kit,panel-version-value", &disp_info->panel_version.value, disp_info->panel_version.value_number);
		disp_info->panel_version.version_number = disp_info->panel_version.value.cnt;

		LCD_KIT_INFO("Panel version value_number = %d version_number = %d \n", disp_info->panel_version.value_number,disp_info->panel_version.version_number);
		if (disp_info->panel_version.version_number > 0) {
			ret = of_property_read_string_array(np, "lcd-kit,panel-version", (const char **)&name[0], disp_info->panel_version.version_number);

			if (ret < 0) {
				LCD_KIT_INFO("Panel version parse fail\n");
			}
			for (i = 0; i < (int)disp_info->panel_version.version_number; i++) {
				strncpy(disp_info->panel_version.lcd_version_name[i], name[i], LCD_PANEL_VERSION_SIZE - 1);
				LCD_KIT_INFO("Panel version is  lcd_version_name[%d] = %s\n", i,name[i]);
			}
		}
	}
	return;
}
/*lint -restore*/
void lcd_kit_parse_dt(struct device_node* np)
{
	if (!np) {
		LCD_KIT_ERR("np is null\n");
		return ;
	}
	/*parse running test*/
	lcd_kit_parse_running(np);
	/*parse effect info*/
	lcd_kit_parse_effect(np);
	/*parse normal function*/
	lcd_kit_parse_util(np);
}

int lcd_kit_checksum_set(struct hisi_fb_data_type* hisifd, int pic_index)
{
	int ret = LCD_KIT_OK;

	if (disp_info->checksum.status == LCD_KIT_CHECKSUM_END) {
		if (TEST_PIC_0 == pic_index) {
			LCD_KIT_INFO("start gram checksum...\n");
			disp_info->checksum.status = LCD_KIT_CHECKSUM_START;
			lcd_kit_effect_switch_ctrl(hisifd, 1);
			lcd_kit_dsi_cmds_tx(hisifd, &disp_info->checksum.enable_cmds);
			disp_info->checksum.pic_index = 0xff;
			LCD_KIT_INFO("Enable checksum\n");
			return ret;
		} else {
			LCD_KIT_INFO("pic index error\n");
			return LCD_KIT_FAIL;
		}
	} else {
		if (TEST_PIC_2 == pic_index) {
			disp_info->checksum.check_count++;
		}
		if (CHECKSUM_CHECKCOUNT == disp_info->checksum.check_count) {
			LCD_KIT_INFO("gram checksum pic %d test 5 times, set checkout end\n", pic_index);
			disp_info->checksum.status = LCD_KIT_CHECKSUM_END;
			disp_info->checksum.check_count = 0;
		}
	}
	switch (pic_index) {
		case TEST_PIC_0:
		case TEST_PIC_1:
		case TEST_PIC_2:
			LCD_KIT_INFO("gram checksum set pic [%d]\n", pic_index);
			disp_info->checksum.pic_index = pic_index;
			break;
		default:
			LCD_KIT_INFO("gram checksum set pic [%d] unknown\n", pic_index);
			disp_info->checksum.pic_index = 0xff;
			break;
	}
	return ret;
}

int lcd_kit_checksum_check(struct hisi_fb_data_type* hisifd)
{
	int ret = LCD_KIT_OK;
	int checksum_result = 0;
	int i = 0;
	uint8_t read_value[LCD_KIT_CHECKSUM_SIZE] = {0};

	if (disp_info->checksum.support) {
		switch (disp_info->checksum.pic_index) {
			case TEST_PIC_0:
			case TEST_PIC_1:
			case TEST_PIC_2:
				LCD_KIT_INFO("start check gram checksum:[%d]\n", disp_info->checksum.pic_index);
				break;
			default:
				LCD_KIT_ERR("gram checksum pic num unknown(%d)\n", disp_info->checksum.pic_index);
				ret = LCD_KIT_FAIL;
				return ret;
		}
	}
	lcd_kit_dsi_cmds_rx(hisifd, read_value, &disp_info->checksum.checksum_cmds);
	if (disp_info->checksum.value.buf == NULL)
	{
		LCD_KIT_ERR("Null pointer.\n");
		return LCD_KIT_FAIL;
	}
	for (i = 0; i < disp_info->checksum.checksum_cmds.cmd_cnt; i++) {
		if (read_value[i] != disp_info->checksum.value.buf[i]) {
			LCD_KIT_ERR("gram check error, read_value[%d]:0x%x, disp_info->checksum.value.buf[%d]:0x%x\n",
						i, read_value[i], i, disp_info->checksum.value.buf[i]);
			checksum_result++;
		}
	}
	if (checksum_result) {
		LCD_KIT_ERR("checksum_result:%d\n", checksum_result);
		checksum_result = 1;
	}
	if (checksum_result && TEST_PIC_2 == disp_info->checksum.pic_index) {
		disp_info->checksum.status = LCD_KIT_CHECKSUM_END;
	}

	if (LCD_KIT_CHECKSUM_END == disp_info->checksum.status) {
		lcd_kit_dsi_cmds_tx(hisifd, &disp_info->checksum.disable_cmds);
		lcd_kit_effect_switch_ctrl(hisifd, 0);
		LCD_KIT_INFO("gram checksum end, disable checksum.\n");
	}
	return checksum_result;
}

void lcd_kit_effect_switch_ctrl(struct hisi_fb_data_type* hisifd, bool ctrl)
{
#if 0
	struct hisi_panel_info* pinfo = NULL;
	char __iomem* dpp_base = NULL;
	char __iomem* lcp_base = NULL;
	char __iomem* gamma_base = NULL;

	if ( NULL == hisifd ) {
		LCD_KIT_ERR("hisifd is null\n");
		return;
	}
	dpp_base = hisifd->dss_base + DSS_DPP_OFFSET;
	lcp_base = hisifd->dss_base + DSS_DPP_LCP_OFFSET;
	gamma_base = hisifd->dss_base + DSS_DPP_GAMA_OFFSET;
	pinfo = &(hisifd->panel_info);
	if (ctrl) {
		if (pinfo->gamma_support == 1) {
			HISI_FB_INFO("disable gamma\n");
			/* disable de-gamma */
			set_reg(lcp_base + LCP_DEGAMA_EN, 0x0, 1, 0);
			/* disable gamma */
			set_reg(gamma_base + GAMA_EN, 0x0, 1, 0);
		}
		if (pinfo->gmp_support == 1) {
			HISI_FB_INFO("disable gmp\n");
			/* disable gmp */
#if defined (CONFIG_HISI_FB_970)
			set_reg(dpp_base + LCP_GMP_BYPASS_EN, 0x0, 1, 0);
#else
			set_reg(dpp_base + LCP_GMP_BYPASS_EN, 0x1, 1, 0);
#endif
		}
		if (pinfo->xcc_support == 1) {
			HISI_FB_INFO("disable xcc\n");
			/* disable xcc */
#if defined (CONFIG_HISI_FB_970)
			set_reg(lcp_base + LCP_XCC_BYPASS_EN, 0x0, 2, 0);
#else
			set_reg(lcp_base + LCP_XCC_BYPASS_EN, 0x1, 1, 0);
#endif
		}
#if !defined (CONFIG_HISI_FB_970) //kirin970 delete bittext function
		/* disable bittext */
		set_reg(hisifd->dss_base + DSS_DPP_BITEXT0_OFFSET + BIT_EXT0_CTL, 0x0, 1, 0);
#endif
	} else {
		if (pinfo->gamma_support == 1) {
			HISI_FB_INFO("enable gamma\n");
			/* enable de-gamma */
			set_reg(lcp_base + LCP_DEGAMA_EN, 0x1, 1, 0);
			/* enable gamma */
			set_reg(gamma_base + GAMA_EN, 0x1, 1, 0);
		}
		if (pinfo->gmp_support == 1) {
			HISI_FB_INFO("enable gmp\n");
			/* enable gmp */
#if defined (CONFIG_HISI_FB_970)
			set_reg(dpp_base + LCP_GMP_BYPASS_EN, 0x3, 2, 0);
#else
			set_reg(dpp_base + LCP_GMP_BYPASS_EN, 0x0, 1, 0);
#endif
		}
		if (pinfo->xcc_support == 1) {
			HISI_FB_INFO("enable xcc\n");
			/* enable xcc */
#if defined (CONFIG_HISI_FB_970)
			set_reg(lcp_base + LCP_XCC_BYPASS_EN, 0x1, 1, 0);
#else
			set_reg(lcp_base + LCP_XCC_BYPASS_EN, 0x0, 1, 0);
#endif
		}
		/* enable bittext */
#if !defined (CONFIG_HISI_FB_970) //kirin970 delete bittext function
		set_reg(hisifd->dss_base + DSS_DPP_BITEXT0_OFFSET + BIT_EXT0_CTL, 0x1, 1, 0);
#endif
	}
#endif
}

int lcd_kit_is_enter_pt_mode(void)
{
	if (common_info->pt.support) {
		return common_info->pt.mode;
	}
	return LCD_KIT_OK;
}

int lcd_kit_current_det(struct hisi_fb_data_type* hisifd)
{
	ssize_t current_check_result = LCD_KIT_OK;
	uint8_t rd = 0;
	
	if(!disp_info->current_det.support) {
		LCD_KIT_INFO("current detect is not support! return!\n");
		return LCD_KIT_OK;
	}
	lcd_kit_dsi_cmds_rx(hisifd, &rd, &disp_info->current_det.detect_cmds);
	LCD_KIT_INFO("current detect, read value = 0x%x\n", rd);
	if ((rd & disp_info->current_det.value.buf[0]) == 0) {
		current_check_result = LCD_KIT_OK;
		LCD_KIT_ERR("no current over\n");
	}
	else{
		current_check_result = LCD_KIT_FAIL;
		LCD_KIT_ERR("current over:0x%x\n", rd);
	}
	return current_check_result;
}

int lcd_kit_lv_det(struct hisi_fb_data_type* hisifd)
{
	ssize_t lv_check_result = LCD_KIT_OK;
	uint8_t rd = 0;
	
	if(!disp_info->lv_det.support) {
		LCD_KIT_INFO("current detect is not support! return!\n");
		return LCD_KIT_OK;
	}
	lcd_kit_dsi_cmds_rx(hisifd, &rd, &disp_info->lv_det.detect_cmds);
	LCD_KIT_INFO("current detect, read value = 0x%x\n", rd);
	if ((rd & disp_info->lv_det.value.buf[0]) == 0) {
		lv_check_result = LCD_KIT_OK;
		LCD_KIT_ERR("no current over\n");
	}
	else{
		lv_check_result = LCD_KIT_FAIL;
		LCD_KIT_ERR("current over:0x%x\n", rd);
	}
	return lv_check_result;
}

int lcd_kit_read_gamma(struct hisi_fb_data_type* hisifd, uint8_t *read_value)
{
	int ret = LCD_KIT_OK;

	disp_info->gamma_cal.cmds.cmds->payload[1] = (disp_info->gamma_cal.addr >> 8) & 0xff;
	disp_info->gamma_cal.cmds.cmds->payload[0] = disp_info->gamma_cal.addr & 0xff;
	disp_info->gamma_cal.cmds.cmds->dlen = disp_info->gamma_cal.length;
	lcd_kit_dsi_cmds_rx(hisifd, (uint8_t*)read_value, &disp_info->gamma_cal.cmds);
	return ret;
}

void lcd_kit_read_power_status(struct hisi_fb_data_type* hisifd)
{
	uint32_t status = 0;
	uint32_t try_times = 0;

	outp32(hisifd->mipi_dsi0_base + MIPIDSI_GEN_HDR_OFFSET, 0x0A06);
	status = inp32(hisifd->mipi_dsi0_base + MIPIDSI_CMD_PKT_STATUS_OFFSET);
	while (status & 0x10) {
		udelay(50);
		if (++try_times > 100) {
			try_times = 0;
			LCD_KIT_ERR("Read lcd power status timeout!\n");
			break;
		}

		status = inp32(hisifd->mipi_dsi0_base + MIPIDSI_CMD_PKT_STATUS_OFFSET);
	}
	status = inp32(hisifd->mipi_dsi0_base + MIPIDSI_GEN_PLD_DATA_OFFSET);
	LCD_KIT_INFO("LCD Power State = 0x%x.\n", status);
	return ;
}


static u32 xcc_table_def[12] = {0x0, 0x8000, 0x0,0x0,0x0,0x0,0x8000,0x0,0x0,0x0,0x0,0x8000,};
int lcd_kit_parse_switch_cmd(struct hisi_fb_data_type* hisifd, char *command)
{
 	struct hisi_panel_info* pinfo = NULL;

	pinfo = &(hisifd->panel_info);
	if (NULL == pinfo) {
		LCD_KIT_ERR("pinfo is null\n");
		return LCD_KIT_FAIL;	
	}
  	if (!strncmp("sbl:", command, strlen("sbl:"))) {
		if ('0' == command[strlen("sbl:")]) {
			pinfo->sbl_support = 0;
 		} else {
			pinfo->sbl_support = 1;
 		}
	}
	if (!strncmp("xcc_support:", command, strlen("xcc_support:"))) {
		if ('0' == command[strlen("xcc_support:")]) {
			pinfo->xcc_support = 0;
			if (pinfo->xcc_table) {
				pinfo->xcc_table[1] = 0x8000;
				pinfo->xcc_table[6] = 0x8000;
				pinfo->xcc_table[11] = 0x8000;
			}
 		} else {
			pinfo->xcc_support = 1;
			if (pinfo->xcc_table == NULL) {
				pinfo->xcc_table = xcc_table_def;
				pinfo->xcc_table_len = ARRAY_SIZE(xcc_table_def);
			}
 		}
	}
	if (!strncmp("dsi_bit_clk_upt:", command, strlen("dsi_bit_clk_upt:"))) {
		if ('0' == command[strlen("dsi_bit_clk_upt:")]) {
			pinfo->dsi_bit_clk_upt_support = 0;
 		} else {
			pinfo->dsi_bit_clk_upt_support = 1;
 		}
	}
	if (!strncmp("dirty_region_upt:", command, strlen("dirty_region_upt:"))) {
		if ('0' == command[strlen("dirty_region_upt:")]) {
			pinfo->dirty_region_updt_support = 0;
 		} else {
			pinfo->dirty_region_updt_support = 1;
 		}
	}
	if (!strncmp("ifbc_type:", command, strlen("ifbc_type:"))) {
		if ('0' == command[strlen("ifbc_type:")]) {
			if (pinfo->ifbc_type == IFBC_TYPE_VESA3X_SINGLE) {
				//ldi
				pinfo->ldi.h_back_porch *= pinfo->pxl_clk_rate_div;
				pinfo->ldi.h_front_porch *= pinfo->pxl_clk_rate_div;
				pinfo->ldi.h_pulse_width *= pinfo->pxl_clk_rate_div;
				pinfo->pxl_clk_rate_div = 1;
				pinfo->ifbc_type = IFBC_TYPE_NONE;
 			}
		} else if ('7' == command[strlen("ifbc_type:")]) {
			if (pinfo->ifbc_type == IFBC_TYPE_NONE) {
				pinfo->pxl_clk_rate_div = 3;
				//ldi
				pinfo->ldi.h_back_porch /= pinfo->pxl_clk_rate_div;
				pinfo->ldi.h_front_porch /= pinfo->pxl_clk_rate_div;
				pinfo->ldi.h_pulse_width /= pinfo->pxl_clk_rate_div;
				pinfo->ifbc_type = IFBC_TYPE_VESA3X_SINGLE;
 			}
		}
	}
	if (!strncmp("esd_enable:", command, strlen("esd_enable:"))) {
		if ('0' == command[strlen("esd_enable:")]) {
			pinfo->esd_enable = 0;
 		} else {
			pinfo->esd_enable = 1;
 		}
	}
	if (!strncmp("fps_updt_support:", command, strlen("fps_updt_support:"))) {
		if ('0' == command[strlen("fps_updt_support:")]) {
			pinfo->fps_updt_support = 0;
 		} else {
			pinfo->fps_updt_support = 1;
 		}
	}
	if (!strncmp("fps_func_switch:", command, strlen("fps_func_switch:"))) {
		if ('0' == command[strlen("fps_func_switch:")]) {
			disp_info->fps.support = 0;
 		} else {
			disp_info->fps.support = 1;
 		}
	}
	if (!strncmp("blpwm_input_ena:", command, strlen("blpwm_input_ena:"))) {
		if ('0' == command[strlen("blpwm_input_ena:")]) {
			pinfo->blpwm_input_ena = 0;
 		} else {
			pinfo->blpwm_input_ena = 1;
 		}
	}
	if (!strncmp("lane_nums:", command, strlen("lane_nums:"))) {
		if (('1' == command[strlen("lane_nums:")]) && (pinfo->mipi.lane_nums_select_support & DSI_1_LANES_SUPPORT)) {
			pinfo->mipi.lane_nums = DSI_1_LANES;
 		} else if (('2' == command[strlen("lane_nums:")]) && (pinfo->mipi.lane_nums_select_support & DSI_2_LANES_SUPPORT)) {
			pinfo->mipi.lane_nums = DSI_2_LANES;
 		} else if (('3' == command[strlen("lane_nums:")]) && (pinfo->mipi.lane_nums_select_support & DSI_3_LANES_SUPPORT)) {
			pinfo->mipi.lane_nums = DSI_3_LANES;
 		} else {
			pinfo->mipi.lane_nums = DSI_4_LANES;
 		}
	}
	if (!strncmp("panel_effect_support:", command, strlen("panel_effect_support:"))) {
		if ('0' == command[strlen("panel_effect_support:")]) {
			pinfo->panel_effect_support = 0;
 		} else {
			pinfo->panel_effect_support = 1;
 		}
	}
	if (!strncmp("color_temp_rectify_support:", command, strlen("color_temp_rectify_support:"))) {
		if ('0' == command[strlen("color_temp_rectify_support:")]) {
			pinfo->color_temp_rectify_support = 0;
 		} else {
			pinfo->color_temp_rectify_support = 1;
 		}
	}
	if (!strncmp("ddic_rgbw_support:", command, strlen("ddic_rgbw_support:"))) {
		if ('0' == command[strlen("ddic_rgbw_support:")]) {
			pinfo->rgbw_support = 0;
			disp_info->rgbw.support = 0;
 		} else {
			pinfo->rgbw_support = 1;
 		}
	}
	hisifb_display_effect_func_switch(hisifd, command);
	return LCD_KIT_OK;
}

static int lcd_kit_get_project_id(char *buff)
{
	if (buff == NULL) {
		LCD_KIT_ERR("buff is null\n");
		return LCD_KIT_FAIL;
	}
	strncpy(buff, disp_info->project_id.id, strlen(disp_info->project_id.id));
	return LCD_KIT_OK;
}

int lcd_kit_get_pt_station_status(void)
{
	int mode = 0;

	if (common_info->pt.support) {
		mode = common_info->pt.mode;
	}
	LCD_KIT_INFO("mode = %d\n", mode);
	return mode;
}

int lcd_kit_get_online_status(void)
{
	int status = LCD_ONLINE;

	if (!strncmp(disp_info->compatible, LCD_KIT_DEFAULT_PANEL, strlen(disp_info->compatible))) {
		/*panel is online*/
		status = LCD_OFFLINE;
	}
	LCD_KIT_INFO("status = %d\n", status);
	return status;
}

int lcd_kit_get_status_by_type(int type, int *status)
{
	int ret = 0;

	if (status == NULL) {
		LCD_KIT_ERR("status is null\n");
		return LCD_KIT_FAIL;
	}
	switch (type) {
	case LCD_ONLINE_TYPE:
		*status = lcd_kit_get_online_status();
		ret = LCD_KIT_OK;
		break;
	case PT_STATION_TYPE:
		*status = lcd_kit_get_pt_station_status();
		ret = LCD_KIT_OK;
		break;
	default:
		LCD_KIT_ERR("not support type\n");
		ret = LCD_KIT_FAIL;
		break;
	}
	return ret;
}

struct lcd_kit_brightness_color_oeminfo g_brightness_color_oeminfo;
void lcd_kit_read_calicolordata_from_share_mem(struct lcd_kit_brightness_color_oeminfo *oeminfo)
{
	void * vir_addr = 0;

	if(oeminfo == NULL)
	{
		LCD_KIT_ERR("point is NULL!\n");
		return;
	}

	vir_addr = (void *)ioremap_wc(HISI_SUB_RESERVED_BRIGHTNESS_CHROMA_MEM_PHYMEM_BASE, HISI_SUB_RESERVED_BRIGHTNESS_CHROMA_MEM_PHYMEM_SIZE);
	if(vir_addr == NULL)
	{
		LCD_KIT_ERR("mem ioremap error !\n");
		return;
	}
	memcpy((void*)oeminfo, (void*)vir_addr, sizeof(struct lcd_kit_brightness_color_oeminfo));
	iounmap(vir_addr);
	return;
}

struct lcd_kit_brightness_color_oeminfo *lcd_kit_get_brightness_color_oeminfo(void)
{
	return &g_brightness_color_oeminfo;
}

int lcd_kit_realtime_set_xcc(struct hisi_fb_data_type *hisifd, char *buf, size_t count)
{
	ssize_t ret = 0;
	int retval = 0;
	struct hisi_fb_panel_data* pdata = NULL;

	if(NULL == hisifd || buf == NULL) {
		LCD_KIT_ERR("NULL pointer\n");
		return -1;
	}

	pdata = dev_get_platdata(&hisifd->pdev->dev);
	if(NULL == pdata) {
		LCD_KIT_ERR("NULL pointer\n");
		return -1;
	}

	if(pdata->lcd_xcc_store) {
		ret = pdata->lcd_xcc_store(hisifd->pdev, buf, count);
		if(ret == count) {
			retval = 0;
		} else {
			LCD_KIT_ERR("set lcd xcc failed!\n");
			retval = -1;
		}
	} else {
		LCD_KIT_ERR("dpe_lcd_xcc_store is NULL\n");
		retval = -1;
	}
	return retval;
}

void lcd_kit_set_actual_bl_max_nit(void)
{
	common_info->actual_bl_max_nit = g_brightness_color_oeminfo.color_params.white_decay_luminace;
}

void lcd_kit_set_mipi_tx_link(struct hisi_fb_data_type *hisifd, struct lcd_kit_dsi_panel_cmds* cmds)
{
	int i = 0;

	switch (cmds->link_state) {
		case LCD_KIT_DSI_LP_MODE:
			if (is_mipi_cmd_panel(hisifd)) {
				/*gen short cmd write switch low-power,include 0-parameter,1-parameter,2-parameter*/
				set_reg(hisifd->mipi_dsi0_base + MIPIDSI_CMD_MODE_CFG_OFFSET, 0x7, 3, 8);
				/*gen long cmd write switch low-power*/
				set_reg(hisifd->mipi_dsi0_base + MIPIDSI_CMD_MODE_CFG_OFFSET, 0x1, 1, 14);
				/*dcs short cmd write switch low-power,include 0-parameter,1-parameter*/
				set_reg(hisifd->mipi_dsi0_base + MIPIDSI_CMD_MODE_CFG_OFFSET, 0x3, 2, 16);
			} else {
			#if defined(CONFIG_HISI_FB_3650) || defined(CONFIG_HISI_FB_6250) || defined(CONFIG_HISI_FB_3660)
				set_reg(hisifd->mipi_dsi0_base + MIPIDSI_VID_MODE_CFG_OFFSET, 0x1, 1, 15);
			#endif
			}
			break;
		case LCD_KIT_DSI_HS_MODE:
			if (is_mipi_cmd_panel(hisifd)) {
				/*gen short cmd write switch high-speed,include 0-parameter,1-parameter,2-parameter*/
				set_reg(hisifd->mipi_dsi0_base + MIPIDSI_CMD_MODE_CFG_OFFSET, 0x0, 3, 8);
				/*gen long cmd write switch high-speed*/
				set_reg(hisifd->mipi_dsi0_base + MIPIDSI_CMD_MODE_CFG_OFFSET, 0x0, 1, 14);
				/*dcs short cmd write switch high-speed,include 0-parameter,1-parameter*/
				set_reg(hisifd->mipi_dsi0_base + MIPIDSI_CMD_MODE_CFG_OFFSET, 0x0, 2, 16);
			} else {
			#if defined(CONFIG_HISI_FB_3650) || defined(CONFIG_HISI_FB_6250) || defined(CONFIG_HISI_FB_3660)
				set_reg(hisifd->mipi_dsi0_base + MIPIDSI_VID_MODE_CFG_OFFSET, 0x0, 1, 15);
			#endif
			}
			break;
		default:
			LCD_KIT_ERR("not support mode\n");
			break;
	}
}

void lcd_kit_set_mipi_rx_link(struct hisi_fb_data_type *hisifd, struct lcd_kit_dsi_panel_cmds* cmds)
{
	int i = 0;

	switch (cmds->link_state) {
		case LCD_KIT_DSI_LP_MODE:
			if (is_mipi_cmd_panel(hisifd)) {
				/*gen short cmd read switch low-power,include 0-parameter,1-parameter,2-parameter*/
				set_reg(hisifd->mipi_dsi0_base + MIPIDSI_CMD_MODE_CFG_OFFSET, 0x7, 3, 11);
				/*dcs short cmd read switch low-power*/
				set_reg(hisifd->mipi_dsi0_base + MIPIDSI_CMD_MODE_CFG_OFFSET, 0x1, 1, 18);
			} else {
			#if defined(CONFIG_HISI_FB_3650) || defined(CONFIG_HISI_FB_6250) || defined(CONFIG_HISI_FB_3660)
				set_reg(hisifd->mipi_dsi0_base + MIPIDSI_VID_MODE_CFG_OFFSET, 0x1, 1, 15);
			#endif
			}
			break;
		case LCD_KIT_DSI_HS_MODE:
			if (is_mipi_cmd_panel(hisifd)) {
				/*gen short cmd read switch high-speed,include 0-parameter,1-parameter,2-parameter*/
				set_reg(hisifd->mipi_dsi0_base + MIPIDSI_CMD_MODE_CFG_OFFSET, 0x0, 3, 11);
				/*dcs short cmd read switch high-speed*/
				set_reg(hisifd->mipi_dsi0_base + MIPIDSI_CMD_MODE_CFG_OFFSET, 0x0, 1, 18);
			} else {
			#if defined(CONFIG_HISI_FB_3650) || defined(CONFIG_HISI_FB_6250) || defined(CONFIG_HISI_FB_3660)
				set_reg(hisifd->mipi_dsi0_base + MIPIDSI_VID_MODE_CFG_OFFSET, 0x0, 1, 15);
			#endif
			}
			break;
		default:
			LCD_KIT_ERR("not support mode\n");
			break;
	}
}

int lcd_kit_get_value_from_dts(char *compatible, char *dts_name, u32 *value)
{
	struct device_node* np = NULL;

	if (!compatible || !dts_name || !value) {
		LCD_KIT_ERR("null pointer found!\n");
		return LCD_KIT_FAIL;
	}
	np = of_find_compatible_node(NULL, NULL, compatible);
	if (!np) {
		LCD_KIT_ERR("NOT FOUND device node %s!\n", compatible);
		return LCD_KIT_FAIL;
	}
	OF_PROPERTY_READ_U32_RETURN(np, dts_name, value);
	return LCD_KIT_OK;
}

static int lcd_kit_power_monitor_on(void)
{
	return ina231_power_monitor_on();
}

static int lcd_kit_power_monitor_off(void)
{
	return ina231_power_monitor_off();
}

static int lcd_kit_set_vss_by_thermal(void)
{
	int ret = 0;
	struct hisi_fb_data_type *hisifd = NULL;
	struct lcd_kit_panel_ops * panel_ops = NULL;

	hisifd = hisifd_list[PRIMARY_PANEL_IDX];
	if(NULL == hisifd){
		LCD_KIT_ERR("NULL Pointer\n");
		return LCD_KIT_FAIL;
	}

	panel_ops = lcd_kit_panel_get_ops();
	if (panel_ops && panel_ops->lcd_set_vss_by_thermal) {
		hisifb_vsync_disable_enter_idle(hisifd, true);
		hisifb_activate_vsync(hisifd);
		ret = panel_ops->lcd_set_vss_by_thermal((void *)hisifd);
		hisifb_vsync_disable_enter_idle(hisifd, false);
		hisifb_deactivate_vsync(hisifd);
	}

	return ret;
}

struct lcd_kit_ops g_lcd_ops = {
	.lcd_kit_support = lcd_kit_support,
	.get_project_id = lcd_kit_get_project_id,
	.create_sysfs = lcd_kit_create_sysfs,
	.get_status_by_type = lcd_kit_get_status_by_type,
	.get_pt_station_status=lcd_kit_get_pt_station_status,
	.get_panel_power_status = lcd_kit_get_power_status,
	.power_monitor_on = lcd_kit_power_monitor_on,
	.power_monitor_off = lcd_kit_power_monitor_off,
	.set_vss_by_thermal = lcd_kit_set_vss_by_thermal,
};

void lcd_kit_set_mipi_clk(struct hisi_fb_data_type* hisifd, uint32_t clk)
{
	if (NULL == hisifd) {
		LCD_KIT_ERR("NULL Pointer\n");
		return ;
	}
	hisifd->panel_info.mipi.dsi_bit_clk = clk;
	hisifd->panel_info.mipi.dsi_bit_clk_upt = clk;
}

int lcd_kit_utils_init(struct device_node* np, struct hisi_panel_info* pinfo)
{
	/*init sem*/
	sema_init(&disp_info->lcd_kit_sem, 1);
	/*init mipi lock*/
	mutex_init(&disp_info->mipi_lock);
	/*parse display dts*/
	lcd_kit_parse_dt(np);
	/*init hisi pinfo*/
	lcd_kit_pinfo_init(np, pinfo);
	/*parse vesa parameters*/
	lcd_kit_vesa_para_parse(np, pinfo);
	/*init compress config*/
	lcd_kit_compress_config(pinfo->ifbc_type, pinfo);
	/*register lcd ops*/
	lcd_kit_ops_register(&g_lcd_ops);
	/*effect init*/
	lcd_kit_effect_get_data(lcd_kit_get_panel_id(disp_info->product_id, disp_info->compatible), pinfo);

	/*Read gamma data from shared memory*/
	if (disp_info->gamma_cal.support) {
		hisifb_update_gm_from_reserved_mem(pinfo->gamma_lut_table_R, pinfo->gamma_lut_table_G,
			pinfo->gamma_lut_table_B, pinfo->igm_lut_table_R,
			pinfo->igm_lut_table_G, pinfo->igm_lut_table_B);
	}

	if (disp_info->oeminfo.support) {
		if (disp_info->oeminfo.brightness_color_uniform.support) {
			lcd_kit_read_calicolordata_from_share_mem(&g_brightness_color_oeminfo);
			lcd_kit_set_actual_bl_max_nit();
		}
	}
	return LCD_KIT_OK;
}
