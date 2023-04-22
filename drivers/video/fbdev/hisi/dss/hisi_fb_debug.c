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

#include "hisi_fb.h"
#include "lcdkit_fb_util.h"
#include "lcdkit_panel.h"
/*
** for debug, S_IRUGO
** /sys/module/hisifb/parameters
*/
/*lint -save -e21 -e846 -e514 -e528 -e708 -e753 -e778 -e866 -e84 -e753 -e528*/
unsigned hisi_fb_msg_level = 7;

int g_debug_mmu_error = 0;

int g_debug_ldi_underflow = 0;

int g_debug_ldi_underflow_clear = 1;

int g_debug_panel_mode_switch = 0;

int g_debug_set_reg_val = 0;

int g_debug_online_vsync = 0;

int g_debug_online_vactive = 0;

int g_debug_ovl_online_composer = 0;

int g_debug_ovl_online_composer_hold = 0;

int g_debug_ovl_online_composer_return = 0;

int g_debug_ovl_online_composer_timediff = 0x0;

int g_debug_ovl_online_composer_time_threshold = 60000;  //us

int g_debug_ovl_offline_composer = 0;

int g_debug_ovl_block_composer = 0;

int g_debug_ovl_offline_composer_hold = 0;

int g_debug_ovl_offline_composer_timediff = 0;

int g_debug_ovl_offline_composer_time_threshold = 12000;  //us

int g_debug_ovl_offline_block_num = -1;

int g_debug_ovl_copybit_composer_hold = 0;

int g_debug_ovl_copybit_composer_timediff = 0;

int g_debug_ovl_copybit_composer_time_threshold = 12000;  //us

int g_debug_ovl_copybit_composer = 0;

int g_debug_ovl_mediacommon_composer = 0;

int g_debug_ovl_cmdlist = 0;

int g_dump_cmdlist_content = 0;

int g_enable_ovl_cmdlist_online = 1;

int g_enable_video_idle_l3cache = 1;

int g_debug_ovl_online_wb_count = 0;

int g_smmu_global_bypass = 0;

int g_enable_ovl_cmdlist_offline = 1;

int g_rdma_stretch_threshold = RDMA_STRETCH_THRESHOLD;

int g_enable_dirty_region_updt = 1;

int g_debug_dirty_region_updt = 0;

int g_enable_mmbuf_debug = 0;

int g_ldi_data_gate_en = 1;

int g_debug_ovl_credit_step = 0;

int g_debug_layerbuf_sync = 0;

int g_debug_offline_layerbuf_sync = 0;

int g_debug_fence_timeline = 0;

int g_enable_dss_idle = 1;

unsigned int g_dss_smmu_outstanding = DSS_SMMU_OUTSTANDING_VAL + 1;

int g_debug_dump_mmbuf = 0;

uint32_t g_underflow_stop_perf_stat = 0;

uint32_t g_dss_min_bandwidth_inbusbusy = 200; //200M

uint32_t g_mmbuf_addr_test = 0;

uint32_t g_dump_sensorhub_aod_hwlock = 0;
/* lint -restore */

//lint -e305, -e514, -e84, -e21, -e846, -e778, -e866, -e708
int g_dss_effect_sharpness1D_en = 1;

int g_dss_effect_sharpness2D_en = 0;

int g_dss_effect_acm_ce_en = 1;

//lint +e305, +e514, +e84, +e21, +e846, +e778, +e866, +e708


static struct dsm_dev dsm_lcd = {
	.name = "dsm_lcd",
	.device_name = NULL,
	.ic_name = NULL,
	.module_name = NULL,
	.fops = NULL,
	.buff_size = 1024,
};

struct dsm_client *lcd_dclient = NULL;

void dss_underflow_debug_func(struct work_struct *work)
{
	struct clk *ddr_clk = NULL;
	unsigned long curr_ddr = 0;
	struct hisi_fb_data_type *hisifd = NULL;
	static u32 underflow_index = 0;
	static ktime_t underflow_timestamp[UNDERFLOW_EXPIRE_COUNT];
	s64 underflow_msecs = 0;
	static bool init_underflow_timestamp = false;
	int i;
	u32 dpp_dbg_value = 0;

	if (NULL == work ) {
		HISI_FB_ERR("work is NULL");
		return;
	}

	if (!init_underflow_timestamp) {
		underflow_timestamp[underflow_index] = ktime_get();
		underflow_index ++;
	}
	if (underflow_index == UNDERFLOW_EXPIRE_COUNT) {
		init_underflow_timestamp = true;
		underflow_timestamp[UNDERFLOW_EXPIRE_COUNT - 1] = ktime_get();
		underflow_msecs = ktime_to_ms(underflow_timestamp[UNDERFLOW_EXPIRE_COUNT - 1]) - ktime_to_ms(underflow_timestamp[0]);
		for(i = 0; i < UNDERFLOW_EXPIRE_COUNT - 1; i ++)
			underflow_timestamp[i] = underflow_timestamp[i+1];
	}

	ddr_clk = clk_get(NULL, "clk_ddrc_freq");
	if (ddr_clk) {
		curr_ddr = clk_get_rate(ddr_clk);
		clk_put(ddr_clk);
	} else {
		HISI_FB_ERR("Get ddr clk failed");
	}

	hisifd = container_of(work, struct hisi_fb_data_type, dss_underflow_debug_work);
	if (hisifd == NULL) {
		HISI_FB_ERR("hisifd is NULL");
		return;
	}

	if (g_underflow_stop_perf_stat) {
		dumpDssOverlay(hisifd, &hisifd->ov_req);
	}

	HISI_FB_INFO("Current ddr is %lu\n", curr_ddr);

	if ((underflow_msecs <= UNDERFLOW_INTERVAL) && (underflow_msecs > 0)) {
		HISI_FB_INFO("abnormal, underflow times:%d, interval:%llu, expire interval:%d\n",
			UNDERFLOW_EXPIRE_COUNT, underflow_msecs, UNDERFLOW_INTERVAL);
	} else {
		HISI_FB_INFO("normal, underflow times:%d, interval:%llu, expire interval:%d\n",
			UNDERFLOW_EXPIRE_COUNT, underflow_msecs, UNDERFLOW_INTERVAL);
		return;
	}

	if (lcd_dclient) {
		if (!dsm_client_ocuppy(lcd_dclient)) {
			if (hisifd->index == PRIMARY_PANEL_IDX) {
				hisifb_activate_vsync(hisifd);
				dpp_dbg_value = inp32(hisifd->dss_base + DSS_DPP_OFFSET + DPP_DBG_CNT);
				hisifb_deactivate_vsync(hisifd);
				dsm_client_record(lcd_dclient,"ldi underflow, curr_ddr = %u, frame_no = %d, dpp_dbg = 0x%x!\n",
					curr_ddr, hisifd->ov_req.frame_no, dpp_dbg_value);
			}
			else {
				dsm_client_record(lcd_dclient, "ldi underflow!\n");
			}
			dsm_client_notify(lcd_dclient, DSM_LCD_LDI_UNDERFLOW_NO);
		}
	}
}

void hisifb_debug_register(struct platform_device *pdev)
{
	struct lcdkit_panel_data *panel;
	struct hisi_fb_data_type *hisifd = NULL;

	if (NULL == pdev) {
		HISI_FB_ERR("pdev is NULL");
		return;
	}
	hisifd = platform_get_drvdata(pdev);
	if (NULL == hisifd) {
		dev_err(&pdev->dev, "hisifd is NULL");
		return;
	}

	// dsm lcd
	if(!lcd_dclient) {
        if (get_lcdkit_support() && PRIMARY_PANEL_IDX == hisifd->index) {
            panel = lcdkit_get_panel_info();
            if (panel && panel->panel_infos.panel_model) {
                dsm_lcd.module_name = panel->panel_infos.panel_model;
            }else if (panel && panel->panel_infos.panel_name) {
                dsm_lcd.module_name = panel->panel_infos.panel_name;
            }
        }

		lcd_dclient = dsm_register_client(&dsm_lcd);
	}

	// dss underflow debug
	hisifd->dss_underflow_debug_workqueue = create_singlethread_workqueue("dss_underflow_debug");
	if (!hisifd->dss_underflow_debug_workqueue) {
		dev_err(&pdev->dev, "fb%d, create dss underflow debug workqueue failed!\n", hisifd->index);
	} else {
		INIT_WORK(&hisifd->dss_underflow_debug_work, dss_underflow_debug_func);
	}
}

void hisifb_debug_unregister(struct platform_device *pdev)
{
	struct hisi_fb_data_type *hisifd = NULL;

	if (NULL == pdev) {
		HISI_FB_ERR("pdev is NULL");
		return;
	}
	hisifd = platform_get_drvdata(pdev);
	if (NULL == hisifd) {
		dev_err(&pdev->dev, "hisifd is NULL");
		return;
	}
}
