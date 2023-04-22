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

#include "hisi_overlay_utils.h"
#include "hisi_dpe_utils.h"


static int hisi_get_ov_data_from_user(struct hisi_fb_data_type *hisifd,
	dss_overlay_t *pov_req, void __user *argp)
{
	int ret = 0;
	dss_overlay_block_t *pov_h_block_infos = NULL;

	if (NULL == hisifd) {
		HISI_FB_ERR("hisifd is NULL\n");
		return -EINVAL;
	}
	if (NULL == pov_req) {
		HISI_FB_ERR("pov_req is NULL\n");
		return -EINVAL;
	}

	if (NULL == argp) {
		HISI_FB_ERR("user data is invalid\n");
		return -EINVAL;
	}

	pov_h_block_infos = hisifd->ov_block_infos;

	ret = copy_from_user(pov_req, argp, sizeof(dss_overlay_t));
	if (ret) {
		HISI_FB_ERR("fb%d, copy_from_user failed!\n", hisifd->index);
		return -EINVAL;
	}

	pov_req->release_fence = -1;
	pov_req->retire_fence = -1;

	if ((pov_req->ov_block_nums <= 0) ||
		(pov_req->ov_block_nums > HISI_DSS_OV_BLOCK_NUMS)) {
		HISI_FB_ERR("fb%d, ov_block_nums(%d) is out of range!\n",
			hisifd->index, pov_req->ov_block_nums);
		return -EINVAL;
	}

	ret = copy_from_user(pov_h_block_infos, (dss_overlay_block_t *)pov_req->ov_block_infos_ptr,
		pov_req->ov_block_nums * sizeof(dss_overlay_block_t));
	if (ret) {
		HISI_FB_ERR("fb%d, dss_overlay_block_t copy_from_user failed!\n",
			hisifd->index);
		return -EINVAL;
	}

	ret = hisi_dss_check_userdata(hisifd, pov_req, pov_h_block_infos);
	if (ret != 0) {
		HISI_FB_ERR("fb%d, hisi_dss_check_userdata failed!\n", hisifd->index);
		return -EINVAL;
	}
	pov_req->ov_block_infos_ptr = (uint64_t)pov_h_block_infos;

	return ret;
}

/*lint -e776 -e737 -e574*/
int hisi_check_vsync_timediff(struct hisi_fb_data_type *hisifd, dss_overlay_t *pov_req)
{
	ktime_t prepare_timestamp;
	uint64_t vsync_timediff;
	uint64_t timestamp;
	int ret;

	if (NULL == hisifd) {
		HISI_FB_ERR("hisi fd is invalid\n");
		return 0;
	}

	ret = hisifb_buf_sync_handle(hisifd, pov_req);
	if (ret < 0) {
		HISI_FB_ERR("fb%d, hisifb_buf_sync_handle failed! ret=%d\n", hisifd->index, ret);
		return -1;
	}

	timestamp = 4000000;
	if (g_dss_version_tag == FB_ACCEL_HI625x) {
		timestamp = 2000000;
	}

	if (is_mipi_video_panel(hisifd)) {
		vsync_timediff = (uint64_t)(hisifd->panel_info.xres + hisifd->panel_info.ldi.h_back_porch +
			hisifd->panel_info.ldi.h_front_porch + hisifd->panel_info.ldi.h_pulse_width) * (hisifd->panel_info.yres +
			hisifd->panel_info.ldi.v_back_porch + hisifd->panel_info.ldi.v_front_porch + hisifd->panel_info.ldi.v_pulse_width) *
			1000000000UL / hisifd->panel_info.pxl_clk_rate;

		prepare_timestamp = ktime_get();
		if ((ktime_to_ns(prepare_timestamp) > ktime_to_ns(hisifd->vsync_ctrl.vsync_timestamp))
			&& ((ktime_to_ns(prepare_timestamp) - ktime_to_ns(hisifd->vsync_ctrl.vsync_timestamp)) < (vsync_timediff - timestamp))
			&& (ktime_to_ns(hisifd->vsync_ctrl.vsync_timestamp_prev) != ktime_to_ns(hisifd->vsync_ctrl.vsync_timestamp))) {
			HISI_FB_DEBUG("fb%d, vsync_timediff=%llu, timestamp_diff=%llu.\n", hisifd->index,
				vsync_timediff, ktime_to_ns(prepare_timestamp) - ktime_to_ns(hisifd->vsync_ctrl.vsync_timestamp));
		} else {
			HISI_FB_DEBUG("fb%d, vsync_timediff=%llu.\n", hisifd->index, vsync_timediff);

			ret = hisi_vactive0_start_config(hisifd, pov_req);
			if (ret != 0) {
				HISI_FB_ERR("fb%d, hisi_vactive0_start_config failed! ret=%d\n", hisifd->index, ret);
				return -1;
			}
		}
	}
	return 0;
}

int hisi_overlay_pan_display(struct hisi_fb_data_type *hisifd)
{
	int ret = 0;
	struct fb_info *fbi = NULL;
	dss_overlay_t *pov_req = NULL;
	dss_overlay_t *pov_req_prev = NULL;
	dss_overlay_block_t *pov_h_block_infos = NULL;
	dss_overlay_block_t *pov_h_block = NULL;
	dss_layer_t *layer = NULL;
	dss_rect_ltrb_t clip_rect;
	dss_rect_t aligned_rect;
	bool rdma_stretch_enable = false;
	uint32_t offset = 0;
	uint32_t addr = 0;
	int hal_format = 0;
	uint32_t cmdlist_pre_idxs = 0;
	uint32_t cmdlist_idxs = 0;
	int enable_cmdlist = 0;
	bool has_base = false;

	if (NULL == hisifd) {
		HISI_FB_ERR("hisi fd is invalid\n");
		return -EINVAL;
	}
	fbi = hisifd->fbi;
	if (NULL == fbi) {
		HISI_FB_ERR("hisifd fbi is invalid\n");
		return -EINVAL;
	}

	pov_req = &(hisifd->ov_req);
	pov_req_prev = &(hisifd->ov_req_prev);

	if (!hisifd->panel_power_on) {
		HISI_FB_INFO("fb%d, panel is power off!", hisifd->index);
		return 0;
	}

	if (g_debug_ldi_underflow) {
		if (g_err_status & (DSS_PDP_LDI_UNDERFLOW | DSS_SDP_LDI_UNDERFLOW))
			return 0;
	}

	offset = fbi->var.xoffset * (fbi->var.bits_per_pixel >> 3) +
		fbi->var.yoffset * fbi->fix.line_length;
	addr = fbi->fix.smem_start + offset;
	if (!fbi->fix.smem_start) {
		HISI_FB_ERR("fb%d, smem_start is null!\n", hisifd->index);
		return -EINVAL;
	}

	if (fbi->fix.smem_len <= 0) {
		HISI_FB_ERR("fb%d, smem_len(%d) is out of range!\n",
			hisifd->index, fbi->fix.smem_len);
		return -EINVAL;
	}

	hal_format = hisi_get_hal_format(fbi);
	if (hal_format < 0) {
		HISI_FB_ERR("fb%d, not support this fb_info's format!\n", hisifd->index);
		return -EINVAL;
	}

	enable_cmdlist = g_enable_ovl_cmdlist_online;
	if ((hisifd->index == EXTERNAL_PANEL_IDX) && hisifd->panel_info.fake_external)
		enable_cmdlist = 0;

	hisifb_activate_vsync(hisifd);

	ret = hisi_vactive0_start_config(hisifd, pov_req);
	if (ret != 0) {
		HISI_FB_ERR("fb%d, hisi_vactive0_start_config failed! ret = %d\n", hisifd->index, ret);
		goto err_return;
	}

	if (g_debug_ovl_online_composer == 1) {
		HISI_FB_INFO("offset=%u.\n", offset);
		dumpDssOverlay(hisifd, pov_req);
	}

	memset(pov_req, 0, sizeof(dss_overlay_t));
	pov_req->ov_block_infos_ptr = (uint64_t)(&(hisifd->ov_block_infos));
	pov_req->ov_block_nums = 1;
	pov_req->ovl_idx = DSS_OVL0;
	pov_req->dirty_rect.x = 0;
	pov_req->dirty_rect.y = 0;
	pov_req->dirty_rect.w = fbi->var.xres;
	pov_req->dirty_rect.h = fbi->var.yres;

	pov_req->res_updt_rect.x = 0;
	pov_req->res_updt_rect.y = 0;
	pov_req->res_updt_rect.w = fbi->var.xres;
	pov_req->res_updt_rect.h = fbi->var.yres;
	pov_req->release_fence = -1;

	pov_h_block_infos = (dss_overlay_block_t *)(pov_req->ov_block_infos_ptr);
	pov_h_block = &(pov_h_block_infos[0]);
	pov_h_block->layer_nums = 1;

	layer = &(pov_h_block->layer_infos[0]);
	layer->img.format = hal_format;
	layer->img.width = fbi->var.xres;
	layer->img.height = fbi->var.yres;
	layer->img.bpp = fbi->var.bits_per_pixel >> 3;
	layer->img.stride = fbi->fix.line_length;
	layer->img.buf_size = layer->img.stride * layer->img.height;
	layer->img.phy_addr = addr;
	layer->img.vir_addr = addr;
	layer->img.mmu_enable = 1;
	layer->img.shared_fd = -1;
	layer->src_rect.x = 0;
	layer->src_rect.y = 0;
	layer->src_rect.w = fbi->var.xres;
	layer->src_rect.h = fbi->var.yres;
	layer->dst_rect.x = 0;
	layer->dst_rect.y = 0;
	layer->dst_rect.w = fbi->var.xres;
	layer->dst_rect.h = fbi->var.yres;
	layer->transform = HISI_FB_TRANSFORM_NOP;
	layer->blending = HISI_FB_BLENDING_NONE;
	layer->glb_alpha = 0xFF;
	layer->color = 0x0;
	layer->layer_idx = 0x0;
	layer->chn_idx = DSS_RCHN_D2;
	layer->need_cap = 0;

	hisi_dss_handle_cur_ovl_req(hisifd, pov_req);

	ret = hisi_dss_module_init(hisifd);
	if (ret != 0) {
		HISI_FB_ERR("fb%d, hisi_dss_module_init failed! ret = %d\n", hisifd->index, ret);
		goto err_return;
	}

	hisi_mmbuf_info_get_online(hisifd);

	if (enable_cmdlist) {
		hisifd->set_reg = hisi_cmdlist_set_reg;

		hisi_cmdlist_data_get_online(hisifd);

		ret = hisi_cmdlist_get_cmdlist_idxs(pov_req_prev, &cmdlist_pre_idxs, NULL);
		if (ret != 0) {
			HISI_FB_ERR("fb%d, hisi_cmdlist_get_cmdlist_idxs pov_req_prev failed! ret = %d\n", hisifd->index, ret);
			goto err_return;
		}

		ret = hisi_cmdlist_get_cmdlist_idxs(pov_req, &cmdlist_pre_idxs, &cmdlist_idxs);
		if (ret != 0) {
			HISI_FB_ERR("fb%d, hisi_cmdlist_get_cmdlist_idxs pov_req failed! ret = %d\n", hisifd->index, ret);
			goto err_return;
		}

		hisi_cmdlist_add_nop_node(hisifd, cmdlist_pre_idxs, 0, 0);
		hisi_cmdlist_add_nop_node(hisifd, cmdlist_idxs, 0, 0);
	} else {
		hisifd->set_reg = hisifb_set_reg;

		hisi_dss_mctl_mutex_lock(hisifd, pov_req->ovl_idx);
		cmdlist_pre_idxs = ~0;
	}

	hisi_dss_prev_module_set_regs(hisifd, pov_req_prev, cmdlist_pre_idxs, enable_cmdlist, NULL);

	hisi_dss_aif_handler(hisifd, pov_req, pov_h_block);

	ret = hisi_dss_ovl_base_config(hisifd, pov_req, NULL, NULL, pov_req->ovl_idx, 0);
	if (ret != 0) {
		HISI_FB_ERR("fb%d, hisi_dss_ovl_init failed! ret = %d\n", hisifd->index, ret);
		goto err_return;
	}

	ret = hisi_ov_compose_handler(hisifd, pov_req, pov_h_block, layer, NULL, NULL,
		&clip_rect, &aligned_rect, &rdma_stretch_enable, &has_base, true, enable_cmdlist);
	if (ret != 0) {
		HISI_FB_ERR("hisi_ov_compose_handler failed! ret = %d\n", ret);
		goto err_return;
	}

	ret = hisi_dss_mctl_ov_config(hisifd, pov_req, pov_req->ovl_idx, has_base, true);
	if (ret != 0) {
		HISI_FB_ERR("fb%d, hisi_dss_mctl_config failed! ret = %d\n", hisifd->index, ret);
		goto err_return;
	}

	if (hisifd->panel_info.dirty_region_updt_support) {
		ret= hisi_dss_dirty_region_dbuf_config(hisifd, pov_req);
		if (ret != 0) {
			HISI_FB_ERR("fb%d, hisi_dss_dirty_region_dbuf_config failed! ret = %d\n", hisifd->index, ret);
			goto err_return;
		}
	}

	ret = hisi_dss_post_scf_config(hisifd, pov_req);
	if (ret != 0) {
		HISI_FB_ERR("fb%d, hisi_dss_post_scf_config failed! ret = %d\n", hisifd->index, ret);
		goto err_return;
	}

	ret = hisi_effect_arsr1p_config(hisifd, pov_req);
	if (ret != 0) {
		HISI_FB_ERR("fb%d, hisi_effect_arsr1p_config failed! ret = %d\n", hisifd->index, ret);
		goto err_return;
	}

	ret = hisi_dss_ov_module_set_regs(hisifd, pov_req, pov_req->ovl_idx, enable_cmdlist, 0, 0, true);
	if (ret != 0) {
		HISI_FB_ERR("fb%d, hisi_dss_module_config failed! ret = %d\n", hisifd->index, ret);
		goto err_return;
	}

	if (!g_fake_lcd_flag) {
		hisi_dss_unflow_handler(hisifd, pov_req, true);
	}

	if (enable_cmdlist) {
		//add taskend for share channel
		hisi_cmdlist_add_nop_node(hisifd, cmdlist_idxs, 0, 0);

		//remove ch cmdlist
		hisi_cmdlist_config_stop(hisifd, cmdlist_pre_idxs);

		cmdlist_idxs |= cmdlist_pre_idxs;
		hisi_cmdlist_flush_cache(hisifd, cmdlist_idxs);

		if (g_debug_ovl_cmdlist) {
			hisi_cmdlist_dump_all_node(hisifd, NULL, cmdlist_idxs);
		}

		if (hisi_cmdlist_config_start(hisifd, pov_req->ovl_idx, cmdlist_idxs, 0)) {
			ret = -EINVAL;
			goto err_return;
		}
	} else {
		hisi_dss_mctl_mutex_unlock(hisifd, pov_req->ovl_idx);
	}

	if (hisifd->panel_info.dirty_region_updt_support) {
		hisi_dss_dirty_region_updt_config(hisifd, pov_req);
	}

	single_frame_update(hisifd);
	hisifb_frame_updated(hisifd);

	hisifb_deactivate_vsync(hisifd);

	hisifd->frame_count++;
	memcpy(&hisifd->ov_req_prev_prev, &hisifd->ov_req_prev, sizeof(dss_overlay_t));
	memcpy(&(hisifd->ov_block_infos_prev_prev), &(hisifd->ov_block_infos_prev),
		hisifd->ov_req_prev.ov_block_nums * sizeof(dss_overlay_block_t));
	hisifd->ov_req_prev_prev.ov_block_infos_ptr = (uint64_t)(&(hisifd->ov_block_infos_prev_prev));

	memcpy(&hisifd->ov_req_prev, pov_req, sizeof(dss_overlay_t));
	memcpy(&(hisifd->ov_block_infos_prev), &(hisifd->ov_block_infos),
		pov_req->ov_block_nums * sizeof(dss_overlay_block_t));
	hisifd->ov_req_prev.ov_block_infos_ptr = (uint64_t)(&(hisifd->ov_block_infos_prev));

	return 0;

err_return:
	if (is_mipi_cmd_panel(hisifd)) {
		hisifd->vactive0_start_flag = 1;
	} else {
		single_frame_update(hisifd);
	}
	hisifb_deactivate_vsync(hisifd);

	return ret;
}

static void wait_pdp_isr_vactive0_end_handle(struct hisi_fb_data_type *hisifd)
{
	uint32_t delaycount = 0;
	uint32_t task_name = 0;
	bool timeout = false;
	struct timeval tv0;
	struct timeval tv1;
	bool time_cost_dbg = false;

	if ((NULL == hisifd) || (hisifd->index != PRIMARY_PANEL_IDX)) {
		return;
	}

	if (PARA_UPDT_DOING == hisifd->panel_info.mode_switch_state) {
		task_name = BIT(1);
	}

	if (task_name) {
		if (time_cost_dbg) {
			hisifb_get_timestamp(&tv0);
		}

		while ((delaycount++) <= 5000) {
			if (task_name & BIT(1)) {
				if (PARA_UPDT_END == hisifd->panel_info.mode_switch_state) {
					break;
				}
			}
			udelay(10);
		}

		if (delaycount > 5000) {
			timeout = true;
		}

		if (time_cost_dbg) {
			hisifb_get_timestamp(&tv1);
			HISI_FB_INFO("timediff is %u us!\n", hisifb_timestamp_diff(&tv0, &tv1));
		}
	}

	if (timeout) {
		HISI_FB_ERR("wait task[0x%x] execution end timeout.\n", task_name);
	}
	return;
}

static void hisifb_mask_layer_backlight_config(struct hisi_fb_data_type *hisifd, dss_overlay_t *pov_req_prev,
	dss_overlay_t *pov_req, bool *masklayer_maxbacklight_flag)
{
	struct hisi_fb_panel_data *pdata = NULL;
	static bool need_max_bl_delay = true;
	static bool need_min_bl_delay = true;
	int mask_delay_time_before_fp = 0;
	int mask_delay_time_after_fp = 0;

	if (hisifd == NULL) {
		HISI_FB_ERR("hisifd is null!\n");
		return;
	}

	if (pov_req == NULL) {
		HISI_FB_DEBUG("pov_req is null!\n");
		return;
	}

	if (pov_req_prev == NULL) {
		HISI_FB_DEBUG("pov_req_prev is null!\n");
		return;
	}

	if (PRIMARY_PANEL_IDX != hisifd->index) {
		HISI_FB_DEBUG("fb%d, not support!\n", hisifd->index);
		return;
	}

	pdata = dev_get_platdata(&hisifd->pdev->dev);
	if (NULL == pdata) {
		HISI_FB_ERR("pdata is NULL");
		return;
	}

	mask_delay_time_before_fp = pdata->panel_info->mask_delay_time_before_fp;
	mask_delay_time_after_fp = pdata->panel_info->mask_delay_time_after_fp;
	if ((pov_req->mask_layer_exist) && !(pov_req_prev->mask_layer_exist) && (pdata->lcd_set_backlight_by_type_func)) {
		HISI_FB_INFO("max backlight %d %d, need_max_bl_delay=%d.\n", pov_req_prev->mask_layer_exist, pov_req->mask_layer_exist, need_max_bl_delay);
		pdata->lcd_set_backlight_by_type_func(hisifd->pdev, 1);
		if (need_max_bl_delay) {
			usleep_range(mask_delay_time_before_fp, mask_delay_time_before_fp);
			need_max_bl_delay = false;
		}
		need_min_bl_delay = true;
		*masklayer_maxbacklight_flag = true;
	}

	if ((pov_req_prev->mask_layer_exist) && !(pov_req->mask_layer_exist) && (pdata->lcd_set_backlight_by_type_func)) {
		HISI_FB_INFO("min backlight %d %d, need_min_bl_delay=%d.\n", pov_req_prev->mask_layer_exist, pov_req->mask_layer_exist, need_min_bl_delay);
		pdata->lcd_set_backlight_by_type_func(hisifd->pdev, 2);
		if (need_min_bl_delay) {
			usleep_range(mask_delay_time_after_fp, mask_delay_time_after_fp);
			need_min_bl_delay = false;
		}
		need_max_bl_delay = true;
		*masklayer_maxbacklight_flag = false;
	}
}

int hisi_ov_online_play(struct hisi_fb_data_type *hisifd, void __user *argp)
{
	static int dss_free_buffer_refcount = 0;
	static bool last_frame_is_idle = 0;
	dss_overlay_t *pov_req = NULL;
	dss_overlay_t *pov_req_prev = NULL;
	dss_overlay_block_t *pov_h_block_infos = NULL;
	dss_overlay_block_t *pov_h_block = NULL;
	dss_layer_t *layer = NULL;
	dss_rect_ltrb_t clip_rect;
	dss_rect_t aligned_rect;
	bool rdma_stretch_enable = false;
	uint32_t cmdlist_pre_idxs = 0;
	uint32_t cmdlist_idxs = 0;
	int enable_cmdlist = 0;
	bool has_base = false;
	unsigned long flags = 0;
	int need_skip = 0;
	int i = 0;
	int m = 0;
	int ret = 0;
	uint32_t timediff = 0;
	bool vsync_time_checked = false;
	bool masklayer_maxbacklight_flag = false;
	struct list_head lock_list;
	struct timeval tv0;
	struct timeval tv1;
	struct timeval tv2;
	struct timeval tv3;
	struct hisi_fb_data_type *fb1;
	int need_wait_1vsync = 0;

	if (NULL == hisifd) {
		HISI_FB_ERR("NULL Pointer!\n");
		return -EINVAL;
	}

	if (NULL == argp) {
		HISI_FB_ERR("NULL Pointer!\n");
		return -EINVAL;
	}


	fb1 = hisifd_list[EXTERNAL_PANEL_IDX];
	pov_req = &(hisifd->ov_req);
	pov_req_prev = &(hisifd->ov_req_prev);
	INIT_LIST_HEAD(&lock_list);

	if (!hisifd->panel_power_on) {
		HISI_FB_INFO("fb%d, panel is power off!\n", hisifd->index);
		hisifd->backlight.bl_updated = 0;
		return 0;
	}

	if (g_debug_ldi_underflow) {
		if (g_err_status & (DSS_PDP_LDI_UNDERFLOW | DSS_SDP_LDI_UNDERFLOW)) {
			mdelay(HISI_DSS_COMPOSER_HOLD_TIME);
			return 0;
		}
	}

	if (g_debug_ovl_online_composer_return) {
		return 0;
	}

	if (g_debug_ovl_online_composer_timediff & 0x2) {
		hisifb_get_timestamp(&tv0);
	}

	enable_cmdlist = g_enable_ovl_cmdlist_online;
	if ((hisifd->index == EXTERNAL_PANEL_IDX) && hisifd->panel_info.fake_external) {
		enable_cmdlist = 0;
	}

	hisifb_activate_vsync(hisifd);

	hisifb_snd_cmd_before_frame(hisifd);

	if (g_debug_ovl_online_composer_timediff & 0x4) {
		hisifb_get_timestamp(&tv2);
	}

	ret = hisi_get_ov_data_from_user(hisifd, pov_req, argp);
	if (ret != 0) {
		HISI_FB_ERR("fb%d, hisi_get_ov_data_from_user failed! ret=%d\n", hisifd->index, ret);
		need_skip = 1;
		goto err_return;
	}

	if ((is_mipi_video_panel(hisifd)) && (hisifd->online_play_count == 1)) {
		need_wait_1vsync = 1;
		HISI_FB_INFO("video panel wait a vsync when first frame displayed! \n");
	}

	if (is_mipi_cmd_panel(hisifd) || (dss_free_buffer_refcount < 2) || need_wait_1vsync) {
		ret = hisi_vactive0_start_config(hisifd, pov_req);
		if (ret != 0) {
			HISI_FB_ERR("fb%d, hisi_vactive0_start_config failed! ret=%d\n", hisifd->index, ret);
			need_skip = 1;
			goto err_return;
		}
	} else if (is_video_idle_ctrl_mode(hisifd)) {
		pov_req->video_idle_status = (pov_req->video_idle_status && last_frame_is_idle) ? 0 : pov_req->video_idle_status;
		if (pov_req->video_idle_status || last_frame_is_idle) {
			ret = hisi_vactive0_start_config(hisifd, pov_req);
			if (ret != 0) {
				HISI_FB_ERR("fb%d, hisi_vactive0_start_config failed! ret=%d\n", hisifd->index, ret);
				need_skip = 1;
				goto err_return;
			}
		}
		last_frame_is_idle = pov_req->video_idle_status ? 1 : 0;
	}

	if (g_debug_ovl_online_composer_timediff & 0x4) {
		hisifb_get_timestamp(&tv3);
		timediff = hisifb_timestamp_diff(&tv2, &tv3);
		if (timediff >= g_debug_ovl_online_composer_time_threshold)
			HISI_FB_ERR("ONLINE_VACTIVE_TIMEDIFF is %u us!\n", timediff);
	}
	down(&hisifd->blank_sem0);

	if (g_debug_ovl_online_composer == 1) {
		dumpDssOverlay(hisifd, pov_req);
	}

	ret = hisifb_layerbuf_lock(hisifd, pov_req, &lock_list);
	if (ret != 0) {
		HISI_FB_ERR("fb%d, hisifb_layerbuf_lock failed! ret=%d\n", hisifd->index, ret);
		goto err_return;
	}

	hisi_dss_handle_cur_ovl_req(hisifd, pov_req);

	ret = hisi_dss_module_init(hisifd);
	if (ret != 0) {
		HISI_FB_ERR("fb%d, hisi_dss_module_init failed! ret = %d\n", hisifd->index, ret);
		goto err_return;
	}

	hisi_mmbuf_info_get_online(hisifd);

	if (enable_cmdlist) {
		hisifd->set_reg = hisi_cmdlist_set_reg;

		hisi_cmdlist_data_get_online(hisifd);

		ret = hisi_cmdlist_get_cmdlist_idxs(pov_req_prev, &cmdlist_pre_idxs, NULL);
		if (ret != 0) {
			HISI_FB_ERR("fb%d, hisi_cmdlist_get_cmdlist_idxs pov_req_prev failed! ret = %d\n", hisifd->index, ret);
			goto err_return;
		}

		ret = hisi_cmdlist_get_cmdlist_idxs(pov_req, &cmdlist_pre_idxs, &cmdlist_idxs);
		if (ret != 0) {
			HISI_FB_ERR("fb%d, hisi_cmdlist_get_cmdlist_idxs pov_req failed! ret = %d\n", hisifd->index, ret);
			goto err_return;
		}

		hisi_cmdlist_add_nop_node(hisifd, cmdlist_pre_idxs, 0, 0);
		hisi_cmdlist_add_nop_node(hisifd, cmdlist_idxs, 0, 0);
	} else {
		hisifd->set_reg = hisifb_set_reg;
		hisi_dss_mctl_mutex_lock(hisifd, pov_req->ovl_idx);
		cmdlist_pre_idxs = ~0;
	}
	hisi_dss_prev_module_set_regs(hisifd, pov_req_prev, cmdlist_pre_idxs, enable_cmdlist, NULL);

	pov_h_block_infos = (dss_overlay_block_t *)(pov_req->ov_block_infos_ptr);
	for (m = 0; m < pov_req->ov_block_nums; m++) {
		pov_h_block = &(pov_h_block_infos[m]);

		ret = hisi_dss_module_init(hisifd);
		if (ret != 0) {
			HISI_FB_ERR("fb%d, hisi_dss_module_init failed! ret = %d\n", hisifd->index, ret);
			goto err_return;
		}

		hisi_dss_aif_handler(hisifd, pov_req, pov_h_block);

		ret = hisi_dss_ovl_base_config(hisifd, pov_req, pov_h_block, NULL, pov_req->ovl_idx, m);
		if (ret != 0) {
			HISI_FB_ERR("fb%d, hisi_dss_ovl_init failed! ret = %d\n", hisifd->index, ret);
			goto err_return;
		}

		/* Go through all layers */
		for (i = 0; i < pov_h_block->layer_nums; i++) {
			layer = &(pov_h_block->layer_infos[i]);
			memset(&clip_rect, 0, sizeof(dss_rect_ltrb_t));
			memset(&aligned_rect, 0, sizeof(dss_rect_ltrb_t));
			rdma_stretch_enable = false;

			ret = hisi_ov_compose_handler(hisifd, pov_req, pov_h_block, layer, NULL, NULL,
				&clip_rect, &aligned_rect, &rdma_stretch_enable, &has_base, true, enable_cmdlist);
			if (ret != 0) {
				HISI_FB_ERR("fb%d, hisi_ov_compose_handler failed! ret = %d\n", hisifd->index, ret);
				goto err_return;
			}
		}

		ret = hisi_dss_mctl_ov_config(hisifd, pov_req, pov_req->ovl_idx, has_base, (m == 0));
		if (ret != 0) {
			HISI_FB_ERR("fb%d, hisi_dss_mctl_config failed! ret = %d\n", hisifd->index, ret);
			goto err_return;
		}

		if (m == 0) {
			if (hisifd->panel_info.dirty_region_updt_support) {
				ret= hisi_dss_dirty_region_dbuf_config(hisifd, pov_req);
				if (ret != 0) {
					HISI_FB_ERR("fb%d, hisi_dss_dirty_region_dbuf_config failed! ret = %d\n", hisifd->index, ret);
					goto err_return;
				}
			}

			ret = hisi_dss_post_scf_config(hisifd, pov_req);
			if (ret != 0) {
				HISI_FB_ERR("fb%d, hisi_dss_post_scf_config failed! ret = %d\n", hisifd->index, ret);
				goto err_return;
			}

			ret = hisi_effect_arsr1p_config(hisifd, pov_req);
			if (ret != 0) {
				HISI_FB_ERR("fb%d, hisi_effect_arsr1p_config failed! ret = %d\n", hisifd->index, ret);
				goto err_return;
			}
			ret = hisi_effect_hiace_config(hisifd);
			if (ret != 0) {
				HISI_FB_ERR("fb%d, hisi_effect_hiace_config failed! ret = %d\n", hisifd->index, ret);
			}

			if ((is_mipi_video_panel(hisifd) && (g_dss_version_tag != FB_ACCEL_HI625x))) {
				vsync_time_checked = true;
				ret = hisi_check_vsync_timediff(hisifd, pov_req);
				if (ret < 0) {
					goto err_return;
				}
			}
		}

		ret = hisi_dss_ov_module_set_regs(hisifd, pov_req, pov_req->ovl_idx, enable_cmdlist, 0, 0, (m == 0));
		if (ret != 0) {
			HISI_FB_ERR("fb%d, hisi_dss_module_config failed! ret = %d\n", hisifd->index, ret);
			goto err_return;
		}
	}
	hisi_sec_mctl_set_regs(hisifd);

	if (enable_cmdlist) {
		g_online_cmdlist_idxs |= cmdlist_idxs;
		//add taskend for share channel
		hisi_cmdlist_add_nop_node(hisifd, cmdlist_idxs, 0, 0);

		//remove ch cmdlist
		hisi_cmdlist_config_stop(hisifd, cmdlist_pre_idxs);

		cmdlist_idxs |= cmdlist_pre_idxs;
		hisi_cmdlist_flush_cache(hisifd, cmdlist_idxs);
	}

	if (!g_fake_lcd_flag) {
		hisi_dss_unflow_handler(hisifd, pov_req, true);
	}

	if (!vsync_time_checked) {
		ret = hisi_check_vsync_timediff(hisifd, pov_req);
		if (ret < 0) {
			goto err_return;
		}
	}

	ret = hisifb_buf_sync_create_fence(hisifd, pov_req);
	if (ret != 0) {
		HISI_FB_INFO("fb%d, hisi_create_fence failed! \n", hisifd->index);
	}

	spin_lock_irqsave(&hisifd->buf_sync_ctrl.refresh_lock, flags);
	hisifd->buf_sync_ctrl.refresh++;
	spin_unlock_irqrestore(&hisifd->buf_sync_ctrl.refresh_lock, flags);

	if (enable_cmdlist) {
		if (hisi_cmdlist_config_start(hisifd, pov_req->ovl_idx, cmdlist_idxs, 0)) {
			hisifb_buf_sync_close_fence(pov_req);
			ret = -EFAULT;
			goto err_return;
		}
	} else {
		hisi_dss_mctl_mutex_unlock(hisifd, pov_req->ovl_idx);
	}

	if (hisifd->panel_info.dirty_region_updt_support) {
		hisi_dss_dirty_region_updt_config(hisifd, pov_req);
	}

	/* cpu config drm layer */
	hisi_drm_layer_online_config(hisifd, pov_req_prev, pov_req);

	hisifb_mask_layer_backlight_config(hisifd, pov_req_prev, pov_req, &masklayer_maxbacklight_flag);

	wait_pdp_isr_vactive0_end_handle(hisifd);

	single_frame_update(hisifd);
	hisifb_frame_updated(hisifd);

	if (copy_to_user((struct dss_overlay_t __user *)argp,
			pov_req, sizeof(dss_overlay_t))) {
		HISI_FB_ERR("fb%d, copy_to_user failed.\n", hisifd->index);
		hisifb_buf_sync_close_fence(pov_req);
		ret = -EFAULT;
		goto err_return;
	}
	/* pass to hwcomposer handle, driver doesn't use it no longer */
	pov_req->release_fence = -1;
	pov_req->retire_fence = -1;

	hisifb_deactivate_vsync(hisifd);

	hisifb_layerbuf_flush(hisifd, &lock_list);

	hisifb_masklayer_backlight_flag_config(hisifd, masklayer_maxbacklight_flag);

	if ((hisifd->index == PRIMARY_PANEL_IDX) && (dss_free_buffer_refcount > 1)) {
		if (!hisifd->fb_mem_free_flag) {
			hisifb_free_fb_buffer(hisifd);
			hisifd->fb_mem_free_flag = true;
		}

		if (g_logo_buffer_base && g_logo_buffer_size) {
			hisifb_free_logo_buffer(hisifd);
			HISI_FB_INFO("dss_free_buffer_refcount = %d !\n", dss_free_buffer_refcount);
		}

		/*wakeup DP when android system has been startup*/
		if (fb1 != NULL) {
			fb1->dp.dptx_gate = true;
			if (fb1->dp_wakeup != NULL) {
				fb1->dp_wakeup(fb1);
			}
		}
	}

	if (g_debug_ovl_online_composer == 2) {
		dumpDssOverlay(hisifd, pov_req);
	}

	if (g_debug_ovl_cmdlist && enable_cmdlist)
		hisi_cmdlist_dump_all_node(hisifd, NULL, cmdlist_idxs);

	hisifd->frame_count++;
	dss_free_buffer_refcount++;
	memcpy(&hisifd->ov_req_prev_prev, &hisifd->ov_req_prev, sizeof(dss_overlay_t));
	memcpy(&(hisifd->ov_block_infos_prev_prev), &(hisifd->ov_block_infos_prev),
		hisifd->ov_req_prev.ov_block_nums * sizeof(dss_overlay_block_t));
	hisifd->ov_req_prev_prev.ov_block_infos_ptr = (uint64_t)(&(hisifd->ov_block_infos_prev_prev));

	memcpy(&hisifd->ov_req_prev, pov_req, sizeof(dss_overlay_t));
	memcpy(&(hisifd->ov_block_infos_prev), &(hisifd->ov_block_infos),
		pov_req->ov_block_nums * sizeof(dss_overlay_block_t));
	hisifd->ov_req_prev.ov_block_infos_ptr = (uint64_t)(&(hisifd->ov_block_infos_prev));
	hisifd->vsync_ctrl.vsync_timestamp_prev = hisifd->vsync_ctrl.vsync_timestamp;

	if (g_debug_ovl_online_composer_timediff & 0x2) {
		hisifb_get_timestamp(&tv1);
		timediff = hisifb_timestamp_diff(&tv0, &tv1);
		if (timediff >= g_debug_ovl_online_composer_time_threshold)  /*lint !e737*/
			HISI_FB_WARNING("ONLINE_TIMEDIFF is %u us!\n", timediff);
	}
	up(&hisifd->blank_sem0);

	return 0;

err_return:
	if (is_mipi_cmd_panel(hisifd)) {
		hisifd->vactive0_start_flag = 1;
	}
	hisifb_layerbuf_lock_exception(hisifd, &lock_list);
	hisifb_deactivate_vsync(hisifd);
	if (!need_skip) {
		up(&hisifd->blank_sem0);
	}
	return ret;
}

/*lint +e776 +e737 +e574*/
