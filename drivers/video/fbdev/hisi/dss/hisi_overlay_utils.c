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
/*lint -e778 -e732 -e845 -e774 -e838 -e438*/

#include "hisi_overlay_utils.h"
#include "hisi_display_effect.h"
#include "hisi_dpe_utils.h"
#include "hisi_ovl_online_wb.h"
// 128 bytes
#define SMMU_RW_ERR_ADDR_SIZE	(128)
static uint32_t vactive_timeout_count = 0;

static struct dss_comm_mmbuf_info g_primary_online_mmbuf[DSS_CHN_MAX_DEFINE] = {{0}};
static struct dss_comm_mmbuf_info g_external_online_mmbuf[DSS_CHN_MAX_DEFINE] = {{0}};

static inline bool hisi_dss_is_sharpness_support(int32_t width, int32_t height)
{
	return ((16 <= width) && (width <= 1600) && (4 <= height) && (height <= 2560));
}


/*******************************************************************************
**
*/
#define DUMP_BUF_SIZE	SZ_256K

struct dss_dump_data_type {
	char *dss_buf;
	uint32_t dss_buf_len;
	char dss_filename[256];

	char *scene_buf;
	uint32_t scene_buf_len;
	char scene_filename[256];

	char image_bin_filename[OVL_LAYER_NUM_MAX][256];
};

void dumpDssOverlay(struct hisi_fb_data_type *hisifd, dss_overlay_t *pov_req)
{
	uint32_t i = 0;
	uint32_t k = 0;
	dss_layer_t const *layer = NULL;
	dss_wb_layer_t const *wb_layer = NULL;
	dss_overlay_block_t *pov_h_block_infos = NULL;
	dss_overlay_block_t *pov_block_info = NULL;

	struct dss_dump_data_type *dumpDss = NULL;

	if (NULL == hisifd) {
		HISI_FB_ERR("hisifd is NULL");
		return;
	}
	if (NULL == pov_req) {
		HISI_FB_ERR("pov_req is NULL");
		return;
	}
	if ((pov_req->ovl_idx < DSS_OVL0) || (pov_req->ovl_idx >= DSS_OVL_IDX_MAX)) {
		HISI_FB_ERR("ovl_idx is invalid");
		return;
	}

	dumpDss = kmalloc(sizeof(struct dss_dump_data_type), GFP_KERNEL);
	if (IS_ERR_OR_NULL(dumpDss)) {
		HISI_FB_ERR("alloc dumpDss failed!\n");
		goto alloc_dump_dss_data_err;
	}
	memset(dumpDss, 0, sizeof(struct dss_dump_data_type));

	dumpDss->dss_buf_len = 0;
	dumpDss->dss_buf = kmalloc(DUMP_BUF_SIZE, GFP_KERNEL);
	if (IS_ERR_OR_NULL(dumpDss->dss_buf)) {
		HISI_FB_ERR("alloc dss_buf failed!\n");
		goto alloc_dss_buf_err;
	}
	memset(dumpDss->dss_buf, 0, DUMP_BUF_SIZE);

	dumpDss->dss_buf_len += snprintf(dumpDss->dss_buf + dumpDss->dss_buf_len, 4 * SZ_1K,
		"\n\n----------------------------<dump begin>----------------------------\n"
		"frame_no=%d\n"
		"ovl_idx=%d\n"
		"res_updt_rect(%d, %d, %d, %d)\n"
		"dirty_rect(%d,%d, %d,%d)\n"
		"release_fence=%d\n"
		"crc_enable_status=%d\n"
		"crc_info(%d,%d)\n"
		"ov_block_nums=%d\n"
		"ov_block_infos_ptr=0x%llx\n"
		"wb_enable=%d\n"
		"wb_layer_nums=%d\n"
		"wb_ov_rect(%d,%d, %d,%d)\n",
		pov_req->frame_no,
		pov_req->ovl_idx,
		pov_req->res_updt_rect.x,
		pov_req->res_updt_rect.y,
		pov_req->res_updt_rect.w,
		pov_req->res_updt_rect.h,
		pov_req->dirty_rect.x,
		pov_req->dirty_rect.y,
		pov_req->dirty_rect.w,
		pov_req->dirty_rect.h,
		pov_req->release_fence,
		pov_req->crc_enable_status,
		pov_req->crc_info.crc_ov_result,
		pov_req->crc_info.err_status,
		pov_req->ov_block_nums,
		pov_req->ov_block_infos_ptr,
		pov_req->wb_enable,
		pov_req->wb_layer_nums,
		pov_req->wb_ov_rect.x,
		pov_req->wb_ov_rect.y,
		pov_req->wb_ov_rect.w,
		pov_req->wb_ov_rect.h);

	for (i = 0; i < pov_req->ov_block_nums; i++) {
		pov_h_block_infos = (dss_overlay_block_t *)(pov_req->ov_block_infos_ptr);
		pov_block_info = &(pov_h_block_infos[i]);

		dumpDss->dss_buf_len += snprintf(dumpDss->dss_buf + dumpDss->dss_buf_len, 4 * SZ_1K,
			"\nov_block_rect(%d,%d, %d,%d)\n"
			"layer_nums=%d\n",
			pov_block_info->ov_block_rect.x,
			pov_block_info->ov_block_rect.y,
			pov_block_info->ov_block_rect.w,
			pov_block_info->ov_block_rect.h,
			pov_block_info->layer_nums);

		for (k = 0; k < pov_block_info->layer_nums; k++) {
			layer = &(pov_block_info->layer_infos[k]);

			dumpDss->dss_buf_len += snprintf(dumpDss->dss_buf + dumpDss->dss_buf_len, 4 * SZ_1K,
				"\nLayerInfo[%d]:\n"
				"format=%d\n"
				"width=%d\n"
				"height=%d\n"
				"bpp=%d\n"
				"buf_size=%d\n"
				"stride=%d\n"
				"stride_plane1=0x%x\n"
				"stride_plane2=0x%x\n"
				"offset_plane1=%d\n"
				"offset_plane2=%d\n"
				"afbc_header_stride=%d\n"
				"afbc_payload_stride=%d\n"
				"afbc_scramble_mode=%d\n"
				"mmbuf_base=0x%x\n"
				"mmbuf_size=%d\n"
				"mmu_enable=%d\n"
				"hfbc_header_stride0=0x%x\n"
				"hfbc_payload_stride0=0x%x\n"
				"hfbc_header_stride1=0x%x\n"
				"hfbc_payload_stride1=0x%x\n"
				"hfbc_scramble_mode=%d\n"
				"csc_mode=%d\n"
				"secure_mode=%d\n"
				"shared_fd=%d\n"
				"src_rect(%d,%d, %d,%d)\n"
				"src_rect_mask(%d,%d, %d,%d)\n"
				"dst_rect(%d,%d, %d,%d)\n"
				"transform=%d\n"
				"blending=%d\n"
				"glb_alpha=0x%x\n"
				"color=0x%x\n"
				"layer_idx=%d\n"
				"chn_idx=%d\n"
				"need_cap=0x%x\n"
				"acquire_fence=%d\n",
				k,
				layer->img.format,
				layer->img.width,
				layer->img.height,
				layer->img.bpp,
				layer->img.buf_size,
				layer->img.stride,
				layer->img.stride_plane1,
				layer->img.stride_plane2,
				layer->img.offset_plane1,
				layer->img.offset_plane2,
				layer->img.afbc_header_stride,
				layer->img.afbc_payload_stride,
				layer->img.afbc_scramble_mode,
				layer->img.mmbuf_base,
				layer->img.mmbuf_size,
				layer->img.mmu_enable,
				layer->img.hfbc_header_stride0,
				layer->img.hfbc_payload_stride0,
				layer->img.hfbc_header_stride1,
				layer->img.hfbc_payload_stride1,
				layer->img.hfbc_scramble_mode,
				layer->img.csc_mode,
				layer->img.secure_mode,
				layer->img.shared_fd,
				layer->src_rect.x,
				layer->src_rect.y,
				layer->src_rect.w,
				layer->src_rect.h,
				layer->src_rect_mask.x,
				layer->src_rect_mask.y,
				layer->src_rect_mask.w,
				layer->src_rect_mask.h,
				layer->dst_rect.x,
				layer->dst_rect.y,
				layer->dst_rect.w,
				layer->dst_rect.h,
				layer->transform,
				layer->blending,
				layer->glb_alpha,
				layer->color,
				layer->layer_idx,
				layer->chn_idx,
				layer->need_cap,
				layer->acquire_fence);
		}
	}

	for (k = 0; k < pov_req->wb_layer_nums; k++) {
		wb_layer = &(pov_req->wb_layer_infos[k]);

		dumpDss->dss_buf_len += snprintf(dumpDss->dss_buf + dumpDss->dss_buf_len, 4 * SZ_1K,
			"\nWbLayerInfo[%d]:\n"
			"format=%d\n"
			"width=%d\n"
			"height=%d\n"
			"bpp=%d\n"
			"buf_size=%d\n"
			"stride=%d\n"
			"stride_plane1=%d\n"
			"stride_plane2=%d\n"
			"offset_plane1=%d\n"
			"offset_plane2=%d\n"
			"afbc_header_stride=%d\n"
			"afbc_payload_stride=%d\n"
			"afbc_scramble_mode=%d\n"
			"mmbuf_base=0x%x\n"
			"mmbuf_size=%d\n"
			"hfbc_header_stride0=0x%x\n"
			"hfbc_payload_stride0=0x%x\n"
			"hfbc_header_stride1=0x%x\n"
			"hfbc_payload_stride1=0x%x\n"
			"hfbc_scramble_mode=%d\n"
			"mmu_enable=%d\n"
			"csc_mode=%d\n"
			"secure_mode=%d\n"
			"shared_fd=%d\n"
			"src_rect(%d,%d, %d,%d)\n"
			"dst_rect(%d,%d, %d,%d)\n"
			"transform=%d\n"
			"chn_idx=%d\n"
			"need_cap=0x%x\n"
			"acquire_fence=%d\n"
			"release_fence=%d\n",
			k,
			wb_layer->dst.format,
			wb_layer->dst.width,
			wb_layer->dst.height,
			wb_layer->dst.bpp,
			wb_layer->dst.buf_size,
			wb_layer->dst.stride,
			wb_layer->dst.stride_plane1,
			wb_layer->dst.stride_plane2,
			wb_layer->dst.offset_plane1,
			wb_layer->dst.offset_plane2,
			wb_layer->dst.afbc_header_stride,
			wb_layer->dst.afbc_payload_stride,
			wb_layer->dst.afbc_scramble_mode,
			wb_layer->dst.mmbuf_base,
			wb_layer->dst.mmbuf_size,
			wb_layer->dst.hfbc_header_stride0,
			wb_layer->dst.hfbc_payload_stride0,
			wb_layer->dst.hfbc_header_stride1,
			wb_layer->dst.hfbc_payload_stride1,
			wb_layer->dst.hfbc_scramble_mode,
			wb_layer->dst.mmu_enable,
			wb_layer->dst.csc_mode,
			wb_layer->dst.secure_mode,
			wb_layer->dst.shared_fd,
			wb_layer->src_rect.x,
			wb_layer->src_rect.y,
			wb_layer->src_rect.w,
			wb_layer->src_rect.h,
			wb_layer->dst_rect.x,
			wb_layer->dst_rect.y,
			wb_layer->dst_rect.w,
			wb_layer->dst_rect.h,
			wb_layer->transform,
			wb_layer->chn_idx,
			wb_layer->need_cap,
			wb_layer->acquire_fence,
			wb_layer->release_fence);
	}

	dumpDss->dss_buf_len += snprintf(dumpDss->dss_buf + dumpDss->dss_buf_len, 4 * SZ_1K,
		"----------------------------<dump end>----------------------------\n\n");

	for (k = 0; k < dumpDss->dss_buf_len; k += 255) {
		printk("%.255s", dumpDss->dss_buf + k);
	}

	if (dumpDss->dss_buf) {
		kfree(dumpDss->dss_buf);
		dumpDss->dss_buf = NULL;
		dumpDss->dss_buf_len = 0;
	}

alloc_dss_buf_err:
	if (dumpDss->scene_buf) {
		kfree(dumpDss->scene_buf);
		dumpDss->scene_buf = NULL;
		dumpDss->scene_buf_len = 0;
	}

       if (dumpDss) {
		kfree(dumpDss);
		dumpDss = NULL;
	}

alloc_dump_dss_data_err:
	return ;
}

int hisifb_get_lcd_id(struct hisi_fb_data_type *hisifd)
{
	int ret = 0;
	struct hisi_fb_panel_data *pdata = NULL;

	if (NULL == hisifd) {
		HISI_FB_ERR("hisifd is NULL");
		return -EINVAL;
	}
	pdata = dev_get_platdata(&hisifd->pdev->dev);
	if (NULL == pdata) {
		HISI_FB_ERR("pdata is NULL");
		return -EINVAL;
	}

	if (pdata->get_lcd_id) {
		ret = pdata->get_lcd_id(hisifd->pdev);
	}
	return ret;
}
static int hisi_dss_lcd_refresh_right_top(dss_layer_t *layer)
{
	int ret = 0;
	if(NULL == layer) {
		HISI_FB_ERR("layer is NULL");
		return -EINVAL;
	}
	switch (layer->transform) {
	case HISI_FB_TRANSFORM_NOP:
		layer->transform = HISI_FB_TRANSFORM_FLIP_H;
		break;
	case HISI_FB_TRANSFORM_FLIP_H:
		layer->transform = HISI_FB_TRANSFORM_NOP;
		break;
	case HISI_FB_TRANSFORM_FLIP_V:
		layer->transform = HISI_FB_TRANSFORM_ROT_180;
		break;
	case HISI_FB_TRANSFORM_ROT_90:
		layer->transform = (HISI_FB_TRANSFORM_ROT_90 | HISI_FB_TRANSFORM_FLIP_H);
		break;
	case HISI_FB_TRANSFORM_ROT_180:
		layer->transform = HISI_FB_TRANSFORM_FLIP_V;
		break;
	case HISI_FB_TRANSFORM_ROT_270:
		layer->transform = (HISI_FB_TRANSFORM_ROT_90 | HISI_FB_TRANSFORM_FLIP_V);
		break;

	case (HISI_FB_TRANSFORM_ROT_90 | HISI_FB_TRANSFORM_FLIP_H):
		layer->transform = HISI_FB_TRANSFORM_ROT_90;
		break;
	case (HISI_FB_TRANSFORM_ROT_90 | HISI_FB_TRANSFORM_FLIP_V):
		layer->transform = HISI_FB_TRANSFORM_ROT_270;
		break;

	default:
		HISI_FB_ERR("not support this transform(%d).\n", layer->transform);
		ret = -1;
		break;
	}
	return ret;
}

static int hisi_dss_lcd_refresh_left_bottom(dss_layer_t *layer)
{
	int ret = 0;
	if(NULL == layer) {
		HISI_FB_ERR("layer is NULL");
		return -EINVAL;
	}
	switch (layer->transform) {
	case HISI_FB_TRANSFORM_NOP:
		layer->transform = HISI_FB_TRANSFORM_FLIP_V;
		break;
	case HISI_FB_TRANSFORM_FLIP_H:
		layer->transform = HISI_FB_TRANSFORM_ROT_180;
		break;
	case HISI_FB_TRANSFORM_FLIP_V:
		layer->transform = HISI_FB_TRANSFORM_NOP;
		break;
	case HISI_FB_TRANSFORM_ROT_90:
		layer->transform = (HISI_FB_TRANSFORM_ROT_90 | HISI_FB_TRANSFORM_FLIP_V);
		break;
	case HISI_FB_TRANSFORM_ROT_180:
		layer->transform = HISI_FB_TRANSFORM_FLIP_H;
		break;
	case HISI_FB_TRANSFORM_ROT_270:
		layer->transform = (HISI_FB_TRANSFORM_ROT_90 | HISI_FB_TRANSFORM_FLIP_H);
		break;

	case (HISI_FB_TRANSFORM_ROT_90 | HISI_FB_TRANSFORM_FLIP_H):
		layer->transform = HISI_FB_TRANSFORM_ROT_270;
		break;
	case (HISI_FB_TRANSFORM_ROT_90 | HISI_FB_TRANSFORM_FLIP_V):
		layer->transform = HISI_FB_TRANSFORM_ROT_90;
		break;

	default:
		HISI_FB_ERR("not support this transform(%d).\n", layer->transform);
		ret = -1;
		break;
	}
	return ret;
}

static int hisi_dss_lcd_refresh_right_bottom(dss_layer_t *layer)
{
	int ret = 0;
	if(NULL == layer) {
		HISI_FB_ERR("layer is NULL");
		return -EINVAL;
	}
	switch (layer->transform) {
	case HISI_FB_TRANSFORM_NOP:
		layer->transform = HISI_FB_TRANSFORM_ROT_180;
		break;
	case HISI_FB_TRANSFORM_FLIP_H:
		layer->transform = HISI_FB_TRANSFORM_FLIP_V;
		break;
	case HISI_FB_TRANSFORM_FLIP_V:
		layer->transform = HISI_FB_TRANSFORM_FLIP_H;
		break;
	case HISI_FB_TRANSFORM_ROT_90:
		layer->transform = HISI_FB_TRANSFORM_ROT_270;
		break;
	case HISI_FB_TRANSFORM_ROT_180:
		layer->transform = HISI_FB_TRANSFORM_NOP;
		break;
	case HISI_FB_TRANSFORM_ROT_270:
		layer->transform = HISI_FB_TRANSFORM_ROT_90;
		break;

	case (HISI_FB_TRANSFORM_ROT_90 | HISI_FB_TRANSFORM_FLIP_H):
		layer->transform = (HISI_FB_TRANSFORM_ROT_90 | HISI_FB_TRANSFORM_FLIP_V);
		break;
	case (HISI_FB_TRANSFORM_ROT_90 | HISI_FB_TRANSFORM_FLIP_V):
		layer->transform = (HISI_FB_TRANSFORM_ROT_90 | HISI_FB_TRANSFORM_FLIP_H);
		break;

	default:
		HISI_FB_ERR("not support this transform(%d).\n", layer->transform);
		ret = -1;
		break;
	}
	return ret;
}

static int hisi_dss_lcd_refresh_direction_layer(struct hisi_fb_data_type *hisifd,
	dss_overlay_t *pov_req, dss_layer_t *layer)
{
	struct hisi_panel_info *pinfo = NULL;
	int ret = 0;

	if (NULL == hisifd) {
		HISI_FB_ERR("hisifd is NULL");
		return -EINVAL;
	}
	if (NULL == pov_req) {
		HISI_FB_ERR("pov_req is NULL");
		return -EINVAL;
	}
	if (NULL == layer) {
		HISI_FB_ERR("layer is NULL");
		return -EINVAL;
	}

	pinfo = &(hisifd->panel_info);

	if ((pov_req->ovl_idx != DSS_OVL0) && (pov_req->ovl_idx != DSS_OVL1))
		return 0;

	if (pinfo->lcd_refresh_direction_ctrl == LCD_REFRESH_LEFT_TOP) {
		; //do nothing
	} else if (pinfo->lcd_refresh_direction_ctrl == LCD_REFRESH_RIGHT_TOP) {
		ret = hisi_dss_lcd_refresh_right_top(layer);
		if (ret == 0) {
			if ((pinfo->dirty_region_updt_support == 1) &&
				(pov_req->dirty_rect.w > 0) &&
				(pov_req->dirty_rect.h > 0)) {
				layer->dst_rect.x = (pov_req->dirty_rect.w - (layer->dst_rect.x + layer->dst_rect.w));
			} else {
				layer->dst_rect.x = (get_panel_xres(hisifd) - (layer->dst_rect.x + layer->dst_rect.w));
			}
		}
	} else if (pinfo->lcd_refresh_direction_ctrl == LCD_REFRESH_LEFT_BOTTOM) {
		ret = hisi_dss_lcd_refresh_left_bottom(layer);
		if (ret == 0) {
			if ((pinfo->dirty_region_updt_support == 1) &&
				(pov_req->dirty_rect.w > 0) &&
				(pov_req->dirty_rect.h > 0)) {
				layer->dst_rect.y = (pov_req->dirty_rect.h - (layer->dst_rect.y + layer->dst_rect.h));
			} else {
				layer->dst_rect.y = (get_panel_yres(hisifd) - (layer->dst_rect.y + layer->dst_rect.h));
			}
		}
	} else if (pinfo->lcd_refresh_direction_ctrl == LCD_REFRESH_RIGHT_BOTTOM) {
		ret = hisi_dss_lcd_refresh_right_bottom(layer);
		if (ret == 0) {
			if ((pinfo->dirty_region_updt_support == 1) &&
				(pov_req->dirty_rect.w > 0) &&
				(pov_req->dirty_rect.h > 0)) {
				layer->dst_rect.x = (pov_req->dirty_rect.w - (layer->dst_rect.x + layer->dst_rect.w));
				layer->dst_rect.y = (pov_req->dirty_rect.h - (layer->dst_rect.y + layer->dst_rect.h));
			} else {
				layer->dst_rect.x = (get_panel_xres(hisifd) - (layer->dst_rect.x + layer->dst_rect.w));
				layer->dst_rect.y = (get_panel_yres(hisifd) - (layer->dst_rect.y + layer->dst_rect.h));
			}
		}
	} else {
		HISI_FB_ERR("fb%d, not support this lcd_refresh_direction_ctrl(%d)!\n",
			hisifd->index, pinfo->lcd_refresh_direction_ctrl);
		ret = -1;
	}

	return ret;
}

static int hisi_dss_lcd_refresh_direction_dirty_region(struct hisi_fb_data_type *hisifd,
	dss_overlay_t *pov_req)
{
	struct hisi_panel_info *pinfo = NULL;
	int ret = 0;

	if (NULL == hisifd) {
		HISI_FB_ERR("hisifd is NULL");
		return -EINVAL;
	}
	if (NULL == pov_req) {
		HISI_FB_ERR("pov_req is NULL");
		return -EINVAL;
	}

	pinfo = &(hisifd->panel_info);

	if ((pov_req->ovl_idx != DSS_OVL0) && (pov_req->ovl_idx != DSS_OVL1))
		return 0;

	if (pinfo->lcd_refresh_direction_ctrl == LCD_REFRESH_LEFT_TOP) {
		; //do nothing
	} else if (pinfo->lcd_refresh_direction_ctrl == LCD_REFRESH_RIGHT_TOP) {
		if (pinfo->dirty_region_updt_support == 1) {
			pov_req->dirty_rect.x =
				(get_panel_xres(hisifd) - (pov_req->dirty_rect.x + pov_req->dirty_rect.w));
		}
	} else if (pinfo->lcd_refresh_direction_ctrl == LCD_REFRESH_LEFT_BOTTOM) {
		if (pinfo->dirty_region_updt_support == 1) {
			pov_req->dirty_rect.y =
				(get_panel_yres(hisifd) - (pov_req->dirty_rect.y + pov_req->dirty_rect.h));
		}
	} else if (pinfo->lcd_refresh_direction_ctrl == LCD_REFRESH_RIGHT_BOTTOM) {
		if (pinfo->dirty_region_updt_support == 1) {
			pov_req->dirty_rect.x =
				(get_panel_xres(hisifd) - (pov_req->dirty_rect.x + pov_req->dirty_rect.w));
			pov_req->dirty_rect.y =
				(get_panel_yres(hisifd) - (pov_req->dirty_rect.y + pov_req->dirty_rect.h));
		}
	} else {
		HISI_FB_ERR("fb%d, not support this lcd_refresh_direction_ctrl(%d)!\n",
			hisifd->index, pinfo->lcd_refresh_direction_ctrl);
		ret = -1;
	}

	return ret;
}

int hisi_dss_handle_cur_ovl_req(struct hisi_fb_data_type *hisifd,
	dss_overlay_t *pov_req)
{
	struct hisi_panel_info *pinfo = NULL;
	dss_overlay_block_t *pov_h_block_infos = NULL;
	dss_overlay_block_t *pov_h_block = NULL;
	dss_layer_t *layer = NULL;
	int i = 0;
	int m = 0;

	if (NULL == hisifd) {
		HISI_FB_ERR("hisifd is NULL");
		return -EINVAL;
	}
	if (NULL == pov_req) {
		HISI_FB_ERR("pov_req is NULL");
		return -EINVAL;
	}
	pinfo = &(hisifd->panel_info);

	hisifd->resolution_rect = pov_req->res_updt_rect;

	pov_h_block_infos = (dss_overlay_block_t *)(pov_req->ov_block_infos_ptr);
	for (m = 0; m < pov_req->ov_block_nums; m++) {
		pov_h_block = &(pov_h_block_infos[m]);

		for (i = 0; i < pov_h_block->layer_nums; i++) {
			layer = &(pov_h_block->layer_infos[i]);

			hisi_dss_lcd_refresh_direction_layer(hisifd, pov_req, layer);
		}
	}

	hisi_dss_lcd_refresh_direction_dirty_region(hisifd, pov_req);

	return 0;
}

/*******************************************************************************
**
*/
int hisi_get_hal_format(struct fb_info *info)
{
	struct fb_var_screeninfo *var = NULL;
	int hal_format = 0;

	if (NULL == info) {
		HISI_FB_ERR("info is NULL \n");
		return -EINVAL;
	}
	var = &info->var;

	switch (var->bits_per_pixel) {
	case 16:
		if (var->blue.offset == 0) {
			if (var->red.offset == 8) {
				hal_format = (var->transp.offset == 12) ?
					HISI_FB_PIXEL_FORMAT_BGRA_4444 : HISI_FB_PIXEL_FORMAT_BGRX_4444;
			} else if (var->red.offset == 10) {
				hal_format = (var->transp.offset == 12) ?
					HISI_FB_PIXEL_FORMAT_BGRA_5551 : HISI_FB_PIXEL_FORMAT_BGRX_5551;
			} else if (var->red.offset == 11) {
				hal_format = HISI_FB_PIXEL_FORMAT_RGB_565;
			} else {
				goto err_return;
			}
		} else {
			if (var->blue.offset == 8) {
				hal_format = (var->transp.offset == 12) ?
					HISI_FB_PIXEL_FORMAT_RGBA_4444 : HISI_FB_PIXEL_FORMAT_RGBX_4444;
			} else if (var->blue.offset == 10) {
				hal_format = (var->transp.offset == 12) ?
					HISI_FB_PIXEL_FORMAT_RGBA_5551 : HISI_FB_PIXEL_FORMAT_RGBX_5551;
			} else if (var->blue.offset == 11) {
				hal_format = HISI_FB_PIXEL_FORMAT_BGR_565;
			} else {
				goto err_return;
			}
		}
		break;

	case 32:
		if (var->blue.offset == 0) {
			hal_format = (var->transp.length == 8) ?
				HISI_FB_PIXEL_FORMAT_BGRA_8888 : HISI_FB_PIXEL_FORMAT_BGRX_8888;
		} else {
			hal_format = (var->transp.length == 8) ?
				HISI_FB_PIXEL_FORMAT_RGBA_8888 : HISI_FB_PIXEL_FORMAT_RGBX_8888;
		}
		break;

	default:
		goto err_return;
	}

	return hal_format;

err_return:
	HISI_FB_ERR("not support this bits_per_pixel(%d)!\n", var->bits_per_pixel);
	return -1;
}

bool hal_format_has_alpha(uint32_t format)
{
	switch (format) {
	case HISI_FB_PIXEL_FORMAT_RGBA_4444:
	case HISI_FB_PIXEL_FORMAT_RGBA_5551:
	case HISI_FB_PIXEL_FORMAT_RGBA_8888:

	case HISI_FB_PIXEL_FORMAT_BGRA_4444:
	case HISI_FB_PIXEL_FORMAT_BGRA_5551:
	case HISI_FB_PIXEL_FORMAT_BGRA_8888:

	case HISI_FB_PIXEL_FORMAT_RGBA_1010102:
	case HISI_FB_PIXEL_FORMAT_BGRA_1010102:
		return true;

	default:
		return false;
	}
}

bool isPixel10Bit2dma (int format)
{
	switch (format) {
	case DMA_PIXEL_FORMAT_RGBA_1010102:
	case DMA_PIXEL_FORMAT_Y410_10BIT:
	case DMA_PIXEL_FORMAT_YUV422_10BIT:
	case DMA_PIXEL_FORMAT_YUV420_SP_10BIT:
	case DMA_PIXEL_FORMAT_YUV422_SP_10BIT:
	case DMA_PIXEL_FORMAT_YUV420_P_10BIT:
	case DMA_PIXEL_FORMAT_YUV422_P_10BIT:
		return true;

	default:
		return false;
	}
}

bool isPixel10Bit2dfc (int format)
{
	switch (format) {
	case DFC_PIXEL_FORMAT_BGRA_1010102:
	case DFC_PIXEL_FORMAT_YUVA_1010102:
	case DFC_PIXEL_FORMAT_UYVA_1010102:
	case DFC_PIXEL_FORMAT_VUYA_1010102:
	case DFC_PIXEL_FORMAT_YUYV_10:
	case DFC_PIXEL_FORMAT_UYVY_10:
		return true;

	default:
		return false;
	}
}

bool isYUVPackage(uint32_t format)
{
	switch (format) {
	case HISI_FB_PIXEL_FORMAT_YUV_422_I:
	case HISI_FB_PIXEL_FORMAT_YUYV_422_Pkg:
	case HISI_FB_PIXEL_FORMAT_YVYU_422_Pkg:
	case HISI_FB_PIXEL_FORMAT_UYVY_422_Pkg:
	case HISI_FB_PIXEL_FORMAT_VYUY_422_Pkg:
	case HISI_FB_PIXEL_FORMAT_YUV422_10BIT:
		return true;

	default:
		return false;
	}
}

bool isYUVSemiPlanar(uint32_t format)
{
	switch (format) {
	case HISI_FB_PIXEL_FORMAT_YCbCr_422_SP:
	case HISI_FB_PIXEL_FORMAT_YCrCb_422_SP:
	case HISI_FB_PIXEL_FORMAT_YCbCr_420_SP:
	case HISI_FB_PIXEL_FORMAT_YCrCb_420_SP:
	case HISI_FB_PIXEL_FORMAT_YCrCb420_SP_10BIT:
	case HISI_FB_PIXEL_FORMAT_YCbCr420_SP_10BIT:
	case HISI_FB_PIXEL_FORMAT_YCbCr422_SP_10BIT:
		return true;

	default:
		return false;
	}
}

bool isYUVPlanar(uint32_t format)
{
	switch (format) {
	case HISI_FB_PIXEL_FORMAT_YCbCr_422_P:
	case HISI_FB_PIXEL_FORMAT_YCrCb_422_P:
	case HISI_FB_PIXEL_FORMAT_YCbCr_420_P:
	case HISI_FB_PIXEL_FORMAT_YCrCb_420_P:
	case HISI_FB_PIXEL_FORMAT_YCbCr420_P_10BIT:
	case HISI_FB_PIXEL_FORMAT_YCbCr422_P_10BIT:
		return true;

	default:
		return false;
	}
}

bool isYUV(uint32_t format)
{
	return isYUVPackage(format) ||
		isYUVSemiPlanar(format) ||
		isYUVPlanar(format);
}

bool is_YUV_SP_420(uint32_t format)
{
	switch (format) {
	case HISI_FB_PIXEL_FORMAT_YCbCr_420_SP:
	case HISI_FB_PIXEL_FORMAT_YCrCb_420_SP:
	case HISI_FB_PIXEL_FORMAT_YCbCr420_SP_10BIT:
	case HISI_FB_PIXEL_FORMAT_YCrCb420_SP_10BIT:
		return true;

	default:
		return false;
	}
}

bool is_YUV_SP_422(uint32_t format)
{
	switch (format) {
	case HISI_FB_PIXEL_FORMAT_YCbCr_422_SP:
	case HISI_FB_PIXEL_FORMAT_YCrCb_422_SP:
		return true;

	default:
		return false;
	}
}

bool is_YUV_P_420(uint32_t format)
{
	switch (format) {
	case HISI_FB_PIXEL_FORMAT_YCbCr_420_P:
	case HISI_FB_PIXEL_FORMAT_YCrCb_420_P:
	case HISI_FB_PIXEL_FORMAT_YCbCr420_P_10BIT:
		return true;

	default:
		return false;
	}
}

bool is_YUV_P_422(uint32_t format)
{
	switch (format) {
	case HISI_FB_PIXEL_FORMAT_YCbCr_422_P:
	case HISI_FB_PIXEL_FORMAT_YCrCb_422_P:
	case HISI_FB_PIXEL_FORMAT_YCbCr422_P_10BIT:
		return true;

	default:
		return false;
	}
}

bool is_RGBX(uint32_t format)
{
	switch (format) {
	case HISI_FB_PIXEL_FORMAT_RGBX_4444:
	case HISI_FB_PIXEL_FORMAT_BGRX_4444:
	case HISI_FB_PIXEL_FORMAT_RGBX_5551:
	case HISI_FB_PIXEL_FORMAT_BGRX_5551:
	case HISI_FB_PIXEL_FORMAT_RGBX_8888:
	case HISI_FB_PIXEL_FORMAT_BGRX_8888:
		return true;

	default:
		return false;
	}
}

bool isNeedDither(int fmt)
{
	return (fmt == DFC_PIXEL_FORMAT_RGB_565) ||
		(fmt == DFC_PIXEL_FORMAT_BGR_565);
}

static bool isNeedRectClip(dss_rect_ltrb_t clip_rect)
{
	return ((clip_rect.left > 0) || (clip_rect.top > 0) ||
		(clip_rect.right > 0) || (clip_rect.bottom > 0));
}

static bool isSrcRectMasked(dss_layer_t *layer, int aligned_pixel)
{
	if (NULL == layer) {
		HISI_FB_ERR("layer is NULL");
		return false;
	}

	return ((layer->src_rect_mask.w != 0) &&
		(layer->src_rect_mask.h != 0) &&
		(ALIGN_DOWN(layer->src_rect_mask.x + layer->src_rect_mask.w, aligned_pixel) > 1));
}

uint32_t isNeedRdmaStretchBlt(struct hisi_fb_data_type *hisifd, dss_layer_t *layer)
{
	uint32_t v_stretch_ratio_threshold = 0;
	uint32_t v_stretch_ratio = 0;

	if (NULL == layer) {
		HISI_FB_ERR("layer is NULL");
		return 0;
	}

	if ((layer->need_cap & CAP_AFBCD) || (layer->need_cap & CAP_HFBCD)) {
		v_stretch_ratio = 0;
	} else {
		if (is_YUV_SP_420(layer->img.format) || is_YUV_P_420(layer->img.format)) {
			v_stretch_ratio_threshold = ((layer->src_rect.h + layer->dst_rect.h - 1) / layer->dst_rect.h);
			v_stretch_ratio = ((layer->src_rect.h / layer->dst_rect.h) / 2) * 2;
		} else {
			v_stretch_ratio_threshold = ((layer->src_rect.h + layer->dst_rect.h - 1) / layer->dst_rect.h);
			v_stretch_ratio = (layer->src_rect.h / layer->dst_rect.h);
		}

		if (v_stretch_ratio_threshold <= g_rdma_stretch_threshold)
			v_stretch_ratio = 0;
	}

	return v_stretch_ratio;
}

void hisifb_dss_overlay_info_init(dss_overlay_t* ov_req)
{
	if (!ov_req)
		return;

	memset(ov_req, 0, sizeof(dss_overlay_t));
	ov_req->release_fence = -1;
	ov_req->retire_fence = -1;
}


int hisi_adjust_clip_rect(dss_layer_t *layer, dss_rect_ltrb_t *clip_rect)
{
	int ret = 0;
	uint32_t temp = 0;

	if (NULL == layer) {
		HISI_FB_ERR("layer is NULL");
		return -EINVAL;
	}
	if (NULL == clip_rect) {
		HISI_FB_ERR("clip_rect is NULL");
		return -EINVAL;
	}

	if ((clip_rect->left < 0 || clip_rect->left > DFC_MAX_CLIP_NUM) ||
		(clip_rect->right < 0 || clip_rect->right > DFC_MAX_CLIP_NUM) ||
		(clip_rect->top < 0 || clip_rect->top > DFC_MAX_CLIP_NUM) ||
		(clip_rect->bottom < 0 || clip_rect->bottom > DFC_MAX_CLIP_NUM)) {
		return -EINVAL;
	}

	switch (layer->transform) {
	case HISI_FB_TRANSFORM_NOP:
		// do nothing
		break;
	case HISI_FB_TRANSFORM_FLIP_H:
	case HISI_FB_TRANSFORM_ROT_90:
		{
			temp = clip_rect->left;
			clip_rect->left = clip_rect->right;
			clip_rect->right = temp;
		}
		break;
	case HISI_FB_TRANSFORM_FLIP_V:
	case HISI_FB_TRANSFORM_ROT_270:
		{
			temp = clip_rect->top;
			clip_rect->top = clip_rect->bottom;
			clip_rect->bottom = temp;
		}
		break;
	case HISI_FB_TRANSFORM_ROT_180:
		{
			temp = clip_rect->left;
			clip_rect->left =  clip_rect->right;
			clip_rect->right = temp ;

			temp = clip_rect->top;
			clip_rect->top =  clip_rect->bottom;
			clip_rect->bottom = temp;
		}
		break;
	default:
		HISI_FB_ERR("not supported this transform(%d)!", layer->transform);
		break;
	}

	return ret;
}

int hisi_pixel_format_hal2dma(int format)
{
	int ret = 0;

	switch(format) {
	case HISI_FB_PIXEL_FORMAT_RGB_565:
	case HISI_FB_PIXEL_FORMAT_BGR_565:
		ret = DMA_PIXEL_FORMAT_RGB_565;
		break;
	case HISI_FB_PIXEL_FORMAT_RGBX_4444:
	case HISI_FB_PIXEL_FORMAT_BGRX_4444:
		ret = DMA_PIXEL_FORMAT_XRGB_4444;
		break;
	case HISI_FB_PIXEL_FORMAT_RGBA_4444:
	case HISI_FB_PIXEL_FORMAT_BGRA_4444:
		ret = DMA_PIXEL_FORMAT_ARGB_4444;
		break;
	case HISI_FB_PIXEL_FORMAT_RGBX_5551:
	case HISI_FB_PIXEL_FORMAT_BGRX_5551:
		ret = DMA_PIXEL_FORMAT_XRGB_5551;
		break;
	case HISI_FB_PIXEL_FORMAT_RGBA_5551:
	case HISI_FB_PIXEL_FORMAT_BGRA_5551:
		ret = DMA_PIXEL_FORMAT_ARGB_5551;
		break;

	case HISI_FB_PIXEL_FORMAT_RGBX_8888:
	case HISI_FB_PIXEL_FORMAT_BGRX_8888:
		ret = DMA_PIXEL_FORMAT_XRGB_8888;
		break;
	case HISI_FB_PIXEL_FORMAT_RGBA_8888:
	case HISI_FB_PIXEL_FORMAT_BGRA_8888:
		ret = DMA_PIXEL_FORMAT_ARGB_8888;
		break;

	case HISI_FB_PIXEL_FORMAT_YUV_422_I:
	case HISI_FB_PIXEL_FORMAT_YUYV_422_Pkg:
	case HISI_FB_PIXEL_FORMAT_YVYU_422_Pkg:
	case HISI_FB_PIXEL_FORMAT_UYVY_422_Pkg:
	case HISI_FB_PIXEL_FORMAT_VYUY_422_Pkg:
		ret = DMA_PIXEL_FORMAT_YUYV_422_Pkg;
		break;

	case HISI_FB_PIXEL_FORMAT_YCbCr_422_P:
	case HISI_FB_PIXEL_FORMAT_YCrCb_422_P:
		ret = DMA_PIXEL_FORMAT_YUV_422_P_HP;
		break;
	case HISI_FB_PIXEL_FORMAT_YCbCr_420_P:
	case HISI_FB_PIXEL_FORMAT_YCrCb_420_P:
		ret = DMA_PIXEL_FORMAT_YUV_420_P_HP;
		break;

	case HISI_FB_PIXEL_FORMAT_YCbCr_422_SP:
	case HISI_FB_PIXEL_FORMAT_YCrCb_422_SP:
		ret = DMA_PIXEL_FORMAT_YUV_422_SP_HP;
		break;
	case HISI_FB_PIXEL_FORMAT_YCbCr_420_SP:
	case HISI_FB_PIXEL_FORMAT_YCrCb_420_SP:
		ret = DMA_PIXEL_FORMAT_YUV_420_SP_HP;
		break;
	case HISI_FB_PIXEL_FORMAT_RGBA_1010102:
	case HISI_FB_PIXEL_FORMAT_BGRA_1010102:
		ret = DMA_PIXEL_FORMAT_RGBA_1010102;
		break;
	case HISI_FB_PIXEL_FORMAT_Y410_10BIT:
		ret = DMA_PIXEL_FORMAT_Y410_10BIT;
		break;
	case HISI_FB_PIXEL_FORMAT_YUV422_10BIT:
		ret = DMA_PIXEL_FORMAT_YUV422_10BIT;
		break;
	case HISI_FB_PIXEL_FORMAT_YCrCb420_SP_10BIT:
	case HISI_FB_PIXEL_FORMAT_YCbCr420_SP_10BIT:
		ret = DMA_PIXEL_FORMAT_YUV420_SP_10BIT;
		break;
	case HISI_FB_PIXEL_FORMAT_YCbCr422_SP_10BIT:
		ret = DMA_PIXEL_FORMAT_YUV422_SP_10BIT;
		break;
	case HISI_FB_PIXEL_FORMAT_YCbCr420_P_10BIT:
		ret = DMA_PIXEL_FORMAT_YUV420_P_10BIT;
		break;
	case HISI_FB_PIXEL_FORMAT_YCbCr422_P_10BIT:
		ret = DMA_PIXEL_FORMAT_YUV422_P_10BIT;
		break;
	default:
		HISI_FB_ERR("not support format(%d)!\n", format);
		ret = -1;
		break;
	}

	return ret;
}
/*lint -e655*/
int hisi_transform_hal2dma(int transform, int chn_idx)
{
	int ret = 0;

	if (chn_idx < DSS_WCHN_W0 || chn_idx == DSS_RCHN_V2) {
		switch(transform) {
		case HISI_FB_TRANSFORM_NOP:
			ret = DSS_TRANSFORM_NOP;
			break;
		case HISI_FB_TRANSFORM_FLIP_H:
			ret = DSS_TRANSFORM_FLIP_H;
			break;
		case HISI_FB_TRANSFORM_FLIP_V:
			ret = DSS_TRANSFORM_FLIP_V;
			break;
		case HISI_FB_TRANSFORM_ROT_180:
			ret = DSS_TRANSFORM_FLIP_V |DSS_TRANSFORM_FLIP_H;
			break;
		case HISI_FB_TRANSFORM_ROT_90:
			ret = DSS_TRANSFORM_ROT | DSS_TRANSFORM_FLIP_H;
			break;
		case HISI_FB_TRANSFORM_ROT_270:
			ret = DSS_TRANSFORM_ROT | DSS_TRANSFORM_FLIP_V;
			break;
		default:
			ret = -1;
			HISI_FB_ERR("Transform %d is not supported", transform);
			break;
		}
	} else {
		if (transform == HISI_FB_TRANSFORM_NOP) {
			ret = DSS_TRANSFORM_NOP;
		} else if (transform == (HISI_FB_TRANSFORM_ROT_90 | HISI_FB_TRANSFORM_FLIP_V)) {
			ret = DSS_TRANSFORM_ROT;
		} else {
			ret = -1;
			HISI_FB_ERR("Transform %d is not supported", transform);
		}
	}

	return ret;
}

static int hisi_pixel_format_hal2dfc(int format)
{
	int ret = 0;

	switch (format) {
	case HISI_FB_PIXEL_FORMAT_RGB_565:
		ret = DFC_PIXEL_FORMAT_RGB_565;
		break;
	case HISI_FB_PIXEL_FORMAT_RGBX_4444:
		ret = DFC_PIXEL_FORMAT_XBGR_4444;
		break;
	case HISI_FB_PIXEL_FORMAT_RGBA_4444:
		ret = DFC_PIXEL_FORMAT_ABGR_4444;
		break;
	case HISI_FB_PIXEL_FORMAT_RGBX_5551:
		ret = DFC_PIXEL_FORMAT_XBGR_5551;
		break;
	case HISI_FB_PIXEL_FORMAT_RGBA_5551:
		ret = DFC_PIXEL_FORMAT_ABGR_5551;
		break;
	case HISI_FB_PIXEL_FORMAT_RGBX_8888:
		ret = DFC_PIXEL_FORMAT_XBGR_8888;
		break;
	case HISI_FB_PIXEL_FORMAT_RGBA_8888:
		ret = DFC_PIXEL_FORMAT_ABGR_8888;
		break;
	case HISI_FB_PIXEL_FORMAT_RGBA_1010102:
		ret = DFC_PIXEL_FORMAT_BGRA_1010102;
		break;
	case HISI_FB_PIXEL_FORMAT_BGR_565:
		ret = DFC_PIXEL_FORMAT_BGR_565;
		break;
	case HISI_FB_PIXEL_FORMAT_BGRX_4444:
		ret = DFC_PIXEL_FORMAT_XRGB_4444;
		break;
	case HISI_FB_PIXEL_FORMAT_BGRA_4444:
		ret = DFC_PIXEL_FORMAT_ARGB_4444;
		break;
	case HISI_FB_PIXEL_FORMAT_BGRX_5551:
		ret = DFC_PIXEL_FORMAT_XRGB_5551;
		break;
	case HISI_FB_PIXEL_FORMAT_BGRA_5551:
		ret = DFC_PIXEL_FORMAT_ARGB_5551;
		break;
	case HISI_FB_PIXEL_FORMAT_BGRX_8888:
		ret = DFC_PIXEL_FORMAT_XRGB_8888;
		break;
	case HISI_FB_PIXEL_FORMAT_BGRA_8888:
		ret = DFC_PIXEL_FORMAT_ARGB_8888;
		break;
	case HISI_FB_PIXEL_FORMAT_BGRA_1010102:
		ret = DFC_PIXEL_FORMAT_BGRA_1010102;
		break;

	case HISI_FB_PIXEL_FORMAT_YUV_422_I:
	case HISI_FB_PIXEL_FORMAT_YUYV_422_Pkg:
		ret = DFC_PIXEL_FORMAT_YUYV422;
		break;
	case HISI_FB_PIXEL_FORMAT_YVYU_422_Pkg:
		ret = DFC_PIXEL_FORMAT_YVYU422;
		break;
	case HISI_FB_PIXEL_FORMAT_UYVY_422_Pkg:
		ret = DFC_PIXEL_FORMAT_UYVY422;
		break;
	case HISI_FB_PIXEL_FORMAT_VYUY_422_Pkg:
		ret = DFC_PIXEL_FORMAT_VYUY422;
		break;

	case HISI_FB_PIXEL_FORMAT_YCbCr_422_SP:
		ret = DFC_PIXEL_FORMAT_YUYV422;
		break;
	case HISI_FB_PIXEL_FORMAT_YCrCb_422_SP:
		ret = DFC_PIXEL_FORMAT_YVYU422;
		break;
	case HISI_FB_PIXEL_FORMAT_YCbCr_420_SP:
		ret = DFC_PIXEL_FORMAT_YUYV422;
		break;
	case HISI_FB_PIXEL_FORMAT_YCrCb_420_SP:
		ret = DFC_PIXEL_FORMAT_YVYU422;
		break;

	case HISI_FB_PIXEL_FORMAT_YCbCr_422_P:
	case HISI_FB_PIXEL_FORMAT_YCbCr_420_P:
		ret = DFC_PIXEL_FORMAT_YUYV422;
		break;
	case HISI_FB_PIXEL_FORMAT_YCrCb_422_P:
	case HISI_FB_PIXEL_FORMAT_YCrCb_420_P:
		ret = DFC_PIXEL_FORMAT_YVYU422;
		break;

	case HISI_FB_PIXEL_FORMAT_Y410_10BIT:
		ret = DFC_PIXEL_FORMAT_UYVA_1010102;
		break;
	case HISI_FB_PIXEL_FORMAT_YCrCb420_SP_10BIT:
	case HISI_FB_PIXEL_FORMAT_YUV422_10BIT:
	case HISI_FB_PIXEL_FORMAT_YCbCr420_SP_10BIT:
	case HISI_FB_PIXEL_FORMAT_YCbCr422_SP_10BIT:
	case HISI_FB_PIXEL_FORMAT_YCbCr420_P_10BIT:
	case HISI_FB_PIXEL_FORMAT_YCbCr422_P_10BIT:
		ret = DFC_PIXEL_FORMAT_YUYV_10;
		break;
	default:
		HISI_FB_ERR("not support format(%d)!\n", format);
		ret = -1;
		break;
	}

	return ret;
}
/*lint +e655*/
static int hisi_rb_swap(int format)
{
	switch (format) {
	case HISI_FB_PIXEL_FORMAT_BGR_565:
	case HISI_FB_PIXEL_FORMAT_BGRX_4444:
	case HISI_FB_PIXEL_FORMAT_BGRA_4444:
	case HISI_FB_PIXEL_FORMAT_BGRX_5551:
	case HISI_FB_PIXEL_FORMAT_BGRA_5551:
	case HISI_FB_PIXEL_FORMAT_BGRX_8888:
	case HISI_FB_PIXEL_FORMAT_BGRA_8888:
	case HISI_FB_PIXEL_FORMAT_BGRA_1010102:
		return 1;
	default:
		return 0;
	}
}

static int hisi_uv_swap(int format)
{
	switch (format) {
	case HISI_FB_PIXEL_FORMAT_YCrCb_422_SP:
	case HISI_FB_PIXEL_FORMAT_YCrCb_420_SP:
	case HISI_FB_PIXEL_FORMAT_YCrCb_422_P:
	case HISI_FB_PIXEL_FORMAT_YCrCb_420_P:
	case HISI_FB_PIXEL_FORMAT_YCrCb420_SP_10BIT:
	case HISI_FB_PIXEL_FORMAT_YCrCb422_SP_10BIT:
		return 1;

	default:
		return 0;
	}
}

static int hisi_dfc_get_bpp(int dfc_format)
{
	int ret = 0;

	switch (dfc_format) {
	case DFC_PIXEL_FORMAT_RGB_565:
	case DFC_PIXEL_FORMAT_XRGB_4444:
	case DFC_PIXEL_FORMAT_ARGB_4444:
	case DFC_PIXEL_FORMAT_XRGB_5551:
	case DFC_PIXEL_FORMAT_ARGB_5551:

	case DFC_PIXEL_FORMAT_BGR_565:
	case DFC_PIXEL_FORMAT_XBGR_4444:
	case DFC_PIXEL_FORMAT_ABGR_4444:
	case DFC_PIXEL_FORMAT_XBGR_5551:
	case DFC_PIXEL_FORMAT_ABGR_5551:
		ret = 2;
		break;

	case DFC_PIXEL_FORMAT_XRGB_8888:
	case DFC_PIXEL_FORMAT_ARGB_8888:
	case DFC_PIXEL_FORMAT_XBGR_8888:
	case DFC_PIXEL_FORMAT_ABGR_8888:
	case DFC_PIXEL_FORMAT_BGRA_1010102:
		ret = 4;
		break;

	case DFC_PIXEL_FORMAT_YUV444:
	case DFC_PIXEL_FORMAT_YVU444:
		ret = 3;
		break;

	case DFC_PIXEL_FORMAT_YUYV422:
	case DFC_PIXEL_FORMAT_YVYU422:
	case DFC_PIXEL_FORMAT_VYUY422:
	case DFC_PIXEL_FORMAT_UYVY422:
	case DFC_PIXEL_FORMAT_YUYV_10:
		ret = 2;
		break;
	case DFC_PIXEL_FORMAT_UYVA_1010102:
		ret = 4;
		break;
	default:
		HISI_FB_ERR("not support format(%d)!\n", dfc_format);
		ret = -1;
		break;
	}

	return ret;
}

static uint32_t hisi_calculate_display_addr(bool mmu_enable, dss_layer_t *layer,
	dss_rect_ltrb_t *aligned_rect, int add_type, bool is_pixel_10bit)
{
	uint32_t addr = 0;
	uint32_t src_addr = 0;
	uint32_t stride = 0;
	uint32_t offset = 0;
	int bpp = 0;
	int left = 0;
	int right = 0;
	int top = 0;
	int bottom = 0;

	left = aligned_rect->left;
	right = aligned_rect->right;
	top = aligned_rect->top;
	bottom = aligned_rect->bottom;

	if (add_type == DSS_ADDR_PLANE0) {
		stride = layer->img.stride;
		offset = 0;
		src_addr = mmu_enable ? layer->img.vir_addr : layer->img.phy_addr;
		bpp = layer->img.bpp;
	} else if (add_type == DSS_ADDR_PLANE1) {
		stride = layer->img.stride_plane1;
		offset = layer->img.offset_plane1;
		src_addr = mmu_enable ? (layer->img.vir_addr + offset) :
			(layer->img.phy_addr + offset);
		bpp = 1;
		if (is_pixel_10bit) {
			bpp = layer->img.bpp;//lint !e713
		}

		if (is_YUV_P_420(layer->img.format) || is_YUV_P_422(layer->img.format)) {
			left /= 2;
			right /= 2;
		}

		if (is_YUV_SP_420(layer->img.format) || is_YUV_P_420(layer->img.format)) {
			top /= 2;
			bottom /= 2;
		}
	} else if (add_type == DSS_ADDR_PLANE2) {
		stride = layer->img.stride_plane2;
		offset = layer->img.offset_plane2;
		src_addr = mmu_enable ? (layer->img.vir_addr + offset) :
			(layer->img.phy_addr + offset);
		bpp = 1;
		if (is_pixel_10bit) {
			bpp = layer->img.bpp;//lint !e713
		}
		if (is_YUV_P_420(layer->img.format) || is_YUV_P_422(layer->img.format)) {
			left /= 2;
			right /= 2;
		}

		if (is_YUV_SP_420(layer->img.format) || is_YUV_P_420(layer->img.format)) {
			top /= 2;
			bottom /= 2;
		}
	} else {
		HISI_FB_ERR("NOT SUPPORT this add_type(%d).\n", add_type);
	}

	switch(layer->transform) {
	case HISI_FB_TRANSFORM_NOP:
		addr = src_addr + top * stride + left * bpp;
		break;
	case HISI_FB_TRANSFORM_FLIP_H:
		addr = src_addr + top * stride + right * bpp;
		break;
	case HISI_FB_TRANSFORM_FLIP_V:
		addr = src_addr + bottom * stride + left * bpp;
		break;
	case HISI_FB_TRANSFORM_ROT_180:
		addr = src_addr + bottom * stride + right * bpp;
		break;
	default:
		HISI_FB_ERR("not supported this transform(%d)!", layer->transform);
		break;
	}

	return addr;
}

/*******************************************************************************
** DSS MIF
*/
void hisi_dss_mif_init(char __iomem *mif_ch_base,
	dss_mif_t *s_mif, int chn_idx)
{
	uint32_t rw_type = 0;

	if (NULL == mif_ch_base) {
		HISI_FB_ERR("mif_ch_base is NULL");
		return;
	}
	if (NULL == s_mif) {
		HISI_FB_ERR("s_mif is NULL");
		return;
	}

	memset(s_mif, 0, sizeof(dss_mif_t));

	s_mif->mif_ctrl1 = 0x00000020;
	s_mif->mif_ctrl2 = 0x0;
	s_mif->mif_ctrl3 = 0x0;
	s_mif->mif_ctrl4 = 0x0;
	s_mif->mif_ctrl5 = 0x0;

	/*
	if (chn_idx == DSS_RCHN_D2) {
		s_mif->mif_ctrl1 = set_bits32(s_mif->mif_ctrl1, 0xA, 4, 0);
	} else {
		s_mif->mif_ctrl1 = set_bits32(s_mif->mif_ctrl1, chn_idx, 4, 0);
	}
	*/
	rw_type = (chn_idx < DSS_WCHN_W0 || chn_idx == DSS_RCHN_V2) ? 0x0 : 0x1;

	s_mif->mif_ctrl1 = set_bits32(s_mif->mif_ctrl1, 0x0, 1, 5);
	s_mif->mif_ctrl1 = set_bits32(s_mif->mif_ctrl1, rw_type, 1, 17);
}

static void hisi_dss_mif_set_reg(struct hisi_fb_data_type *hisifd,
	char __iomem *mif_ch_base, dss_mif_t *s_mif, int chn_idx)
{
	if (hisifd == NULL) {
		HISI_FB_DEBUG("hisifd is NULL!\n");
		return;
	}

	if (mif_ch_base == NULL) {
		HISI_FB_DEBUG("mif_ch_base is NULL!\n");
		return;
	}

	if (s_mif == NULL) {
		HISI_FB_DEBUG("s_mif is NULL!\n");
		return;
	}

	hisifd->set_reg(hisifd, mif_ch_base + MIF_CTRL1,
		s_mif->mif_ctrl1, 32, 0);
	hisifd->set_reg(hisifd, mif_ch_base + MIF_CTRL2,
		s_mif->mif_ctrl2, 32, 0);
	hisifd->set_reg(hisifd, mif_ch_base + MIF_CTRL3,
		s_mif->mif_ctrl3, 32, 0);
	hisifd->set_reg(hisifd, mif_ch_base + MIF_CTRL4,
		s_mif->mif_ctrl4, 32, 0);
	hisifd->set_reg(hisifd, mif_ch_base + MIF_CTRL5,
		s_mif->mif_ctrl5, 32, 0);
}

void hisi_dss_mif_on(struct hisi_fb_data_type *hisifd)
{
	char __iomem *mif_base = NULL;

	if (NULL == hisifd) {
		HISI_FB_ERR("hisifd is NULL");
		return;
	}

	mif_base = hisifd->dss_base + DSS_MIF_OFFSET;

	set_reg(mif_base + MIF_ENABLE, 0x1, 1, 0);
	//set_reg(mif_base + MIF_MEM_CTRL, 0x01a80050, 32, 0);
	//set_reg(mif_base + MIF_CLK_CTL, , 2, 0);

	set_reg(hisifd->dss_base + MIF_CH0_OFFSET + MIF_CTRL0, 0x1, 1, 0);
	set_reg(hisifd->dss_base + MIF_CH1_OFFSET + MIF_CTRL0, 0x1, 1, 0);
	set_reg(hisifd->dss_base + MIF_CH2_OFFSET + MIF_CTRL0, 0x1, 1, 0);
	set_reg(hisifd->dss_base + MIF_CH3_OFFSET + MIF_CTRL0, 0x1, 1, 0);
	set_reg(hisifd->dss_base + MIF_CH4_OFFSET + MIF_CTRL0, 0x1, 1, 0);
	set_reg(hisifd->dss_base + MIF_CH5_OFFSET + MIF_CTRL0, 0x1, 1, 0);
	set_reg(hisifd->dss_base + MIF_CH6_OFFSET + MIF_CTRL0, 0x1, 1, 0);
	set_reg(hisifd->dss_base + MIF_CH7_OFFSET + MIF_CTRL0, 0x1, 1, 0);
	set_reg(hisifd->dss_base + MIF_CH8_OFFSET + MIF_CTRL0, 0x1, 1, 0);
	set_reg(hisifd->dss_base + MIF_CH9_OFFSET + MIF_CTRL0, 0x1, 1, 0);

	set_reg(hisifd->dss_base + MIF_CH10_OFFSET + MIF_CTRL0, 0x1, 1, 0);
	set_reg(hisifd->dss_base + MIF_CH11_OFFSET + MIF_CTRL0, 0x1, 1, 0);

}

int hisi_dss_mif_config(struct hisi_fb_data_type *hisifd,
	dss_layer_t *layer, dss_wb_layer_t *wb_layer, bool rdma_stretch_enable)
{
	dss_mif_t *mif = NULL;
	int chn_idx = 0;
	dss_img_t *img = NULL;
	uint32_t transform = 0;
	uint32_t invalid_sel = 0;
	uint32_t need_cap = 0;
	uint32_t* semi_plane1 = NULL;
	int v_scaling_factor = 0;

	if (NULL == hisifd) {
		HISI_FB_ERR("hisifd is NULL");
		return -EINVAL;
	}
	if ((layer == NULL) && (wb_layer == NULL)) {
		HISI_FB_ERR("layer and wb_layer is NULL");
		return -EINVAL;
	}

	if (wb_layer) {
		img = &(wb_layer->dst);
		chn_idx = wb_layer->chn_idx;
		transform = wb_layer->transform;
		need_cap = wb_layer->need_cap;
		v_scaling_factor = 1;
	} else {
		img = &(layer->img);
		chn_idx = layer->chn_idx;
		transform = layer->transform;
		need_cap = layer->need_cap;
		v_scaling_factor = layer->src_rect.h / layer->dst_rect.h + ((layer->src_rect.h % layer->dst_rect.h) > 0 ? 1 : 0);
	}

	mif = &(hisifd->dss_module.mif[chn_idx]);
	hisifd->dss_module.mif_used[chn_idx] = 1;

	semi_plane1 = &mif->mif_ctrl4;


	if (img->mmu_enable == 0) {
		mif->mif_ctrl1 = set_bits32(mif->mif_ctrl1, 0x1, 1, 5);
	} else {
		if (need_cap & (CAP_AFBCD | CAP_AFBCE | CAP_HFBCD | CAP_HFBCE)) {
			invalid_sel = 0;
		} else {
			invalid_sel = hisi_dss_mif_get_invalid_sel(img, transform, v_scaling_factor, ((need_cap & CAP_TILE) ? 1 : 0), rdma_stretch_enable);
		}

		mif->mif_ctrl1 = set_bits32(mif->mif_ctrl1, 0x0, 1, 5);
		mif->mif_ctrl1 = set_bits32(mif->mif_ctrl1, invalid_sel, 2, 10);
		mif->mif_ctrl1 = set_bits32(mif->mif_ctrl1, ((invalid_sel == 0) ? 0x1 : 0x0), 1, 19);

		if (invalid_sel == 0) {
			mif->mif_ctrl2 = set_bits32(mif->mif_ctrl2, 0x0, 20, 0);
			mif->mif_ctrl3 = set_bits32(mif->mif_ctrl3, 0x0, 20, 0);
			mif->mif_ctrl4 = set_bits32(mif->mif_ctrl4, 0x0, 20, 0);
			mif->mif_ctrl5 = set_bits32(mif->mif_ctrl5, 0x0, 20, 0);
		} else if ((invalid_sel == 1) || (invalid_sel == 2)) {
			if (img->stride > 0) {
				mif->mif_ctrl5 = set_bits32(mif->mif_ctrl5,
					((img->stride / MIF_STRIDE_UNIT) + (((img->stride % MIF_STRIDE_UNIT) > 0) ? 1: 0)), 20, 0);
			}

			if (isYUVSemiPlanar(img->format)) {
				if (img->stride_plane1 > 0) {
					*semi_plane1 = set_bits32(*semi_plane1,
						((img->stride_plane1 / MIF_STRIDE_UNIT) + (((img->stride_plane1 % MIF_STRIDE_UNIT) > 0) ? 1: 0)), 20, 0);
				}
			} else if (isYUVPlanar(img->format)) {
				if (img->stride_plane1 > 0) {
					mif->mif_ctrl4= set_bits32(mif->mif_ctrl4,
						((img->stride_plane1 / MIF_STRIDE_UNIT) + (((img->stride_plane1 % MIF_STRIDE_UNIT) > 0) ? 1: 0)), 20, 0);
				}

				if (img->stride_plane2 > 0) {
					mif->mif_ctrl3 = set_bits32(mif->mif_ctrl3,
						((img->stride_plane2 / MIF_STRIDE_UNIT) + (((img->stride_plane2 % MIF_STRIDE_UNIT) > 0) ? 1: 0)), 20, 0);
				}
			} else {
				;
			}
		} else if (invalid_sel == 3) {
			if (img->stride > 0) {
				mif->mif_ctrl5 = set_bits32(mif->mif_ctrl5, DSS_MIF_CTRL2_INVAL_SEL3_STRIDE_MASK, 4, 16);
			}
			if (isYUVSemiPlanar(img->format)) {
				if (img->stride_plane1 > 0)
					*semi_plane1 = set_bits32(*semi_plane1, 0xE, 4, 16);

			} else if (isYUVPlanar(img->format)) {
				if (img->stride_plane1 > 0)
					mif->mif_ctrl3 = set_bits32(mif->mif_ctrl3, 0xE, 4, 16);

				if (img->stride_plane2 > 0)
					mif->mif_ctrl4 = set_bits32(mif->mif_ctrl4, 0xE, 4, 16);
			} else {
				; //do nothing
			}

			//Tile
			//YUV_SP: RDMA0 128KB aligned, RDMA1 64KB aligned
			//YUV_P: RDMA0 128KB aligned, RDMA1 64KB aligned, RDMA2 64KB aligned
			//ROT: 256KB aligned.
		} else {
			HISI_FB_ERR("fb%d, invalid_sel(%d) not support!\n", hisifd->index, invalid_sel);
		}
	}

	return 0;
}


/*******************************************************************************
** DSS RDMA
*/
void hisi_dss_rdma_init(char __iomem *dma_base, dss_rdma_t *s_dma)
{
	if (NULL == dma_base) {
		HISI_FB_ERR("dma_base is NULL");
		return;
	}
	if (NULL == s_dma) {
		HISI_FB_ERR("s_dma is NULL");
		return;
	}

	memset(s_dma, 0, sizeof(dss_rdma_t));

	s_dma->oft_x0 = inp32(dma_base + DMA_OFT_X0);
	s_dma->oft_y0 = inp32(dma_base + DMA_OFT_Y0);
	s_dma->oft_x1 = inp32(dma_base + DMA_OFT_X1);
	s_dma->oft_y1 = inp32(dma_base + DMA_OFT_Y1);
	s_dma->mask0 = inp32(dma_base + DMA_MASK0);
	s_dma->mask1 = inp32(dma_base + DMA_MASK1);
	s_dma->stretch_size_vrt = inp32(dma_base + DMA_STRETCH_SIZE_VRT);
	s_dma->ctrl = inp32(dma_base + DMA_CTRL);
	s_dma->tile_scram = inp32(dma_base + DMA_TILE_SCRAM);

	s_dma->ch_rd_shadow = inp32(dma_base + CH_RD_SHADOW);
	s_dma->ch_ctl = inp32(dma_base + CH_CTL);

	s_dma->data_addr0 = inp32(dma_base + DMA_DATA_ADDR0);
	s_dma->stride0 = inp32(dma_base + DMA_STRIDE0);
	s_dma->stretch_stride0 = inp32(dma_base + DMA_STRETCH_STRIDE0);
	s_dma->data_num0 = inp32(dma_base + DMA_DATA_NUM0);

	s_dma->vpp_ctrl = inp32(dma_base + VPP_CTRL);
	s_dma->vpp_mem_ctrl = inp32(dma_base + VPP_MEM_CTRL);

	s_dma->dma_buf_ctrl = inp32(dma_base + DMA_BUF_CTRL);

	s_dma->afbcd_hreg_hdr_ptr_lo = inp32(dma_base + AFBCD_HREG_HDR_PTR_LO);
	s_dma->afbcd_hreg_pic_width = inp32(dma_base + AFBCD_HREG_PIC_WIDTH);
	s_dma->afbcd_hreg_pic_height = inp32(dma_base + AFBCD_HREG_PIC_HEIGHT);
	s_dma->afbcd_hreg_format = inp32(dma_base + AFBCD_HREG_FORMAT);
	s_dma->afbcd_ctl = inp32(dma_base + AFBCD_CTL);
	s_dma->afbcd_str = inp32(dma_base + AFBCD_STR);
	s_dma->afbcd_line_crop = inp32(dma_base + AFBCD_LINE_CROP);
	s_dma->afbcd_input_header_stride = inp32(dma_base + AFBCD_INPUT_HEADER_STRIDE);
	s_dma->afbcd_payload_stride = inp32(dma_base + AFBCD_PAYLOAD_STRIDE);
	s_dma->afbcd_mm_base_0 = inp32(dma_base + AFBCD_MM_BASE_0);
	s_dma->afbcd_afbcd_payload_pointer = inp32(dma_base + AFBCD_AFBCD_PAYLOAD_POINTER);
	s_dma->afbcd_height_bf_str = inp32(dma_base + AFBCD_HEIGHT_BF_STR);
	s_dma->afbcd_os_cfg = inp32(dma_base + AFBCD_OS_CFG);
	s_dma->afbcd_mem_ctrl = inp32(dma_base + AFBCD_MEM_CTRL);
}

void hisi_dss_rdma_u_init(char __iomem *dma_base, dss_rdma_t *s_dma)
{
	if (NULL == dma_base) {
		HISI_FB_ERR("dma_base is NULL");
		return;
	}
	if (NULL == s_dma) {
		HISI_FB_ERR("s_dma is NULL");
		return;
	}

	s_dma->data_addr1 = inp32(dma_base + DMA_DATA_ADDR1);
	s_dma->stride1 = inp32(dma_base + DMA_STRIDE1);
	s_dma->stretch_stride1 = inp32(dma_base + DMA_STRETCH_STRIDE1);
	s_dma->data_num1 = inp32(dma_base + DMA_DATA_NUM1);
}

void hisi_dss_rdma_v_init(char __iomem *dma_base, dss_rdma_t *s_dma)
{
	if (NULL == dma_base) {
		HISI_FB_ERR("dma_base is NULL");
		return;
	}
	if (NULL == s_dma) {
		HISI_FB_ERR("s_dma is NULL");
		return;
	}

	s_dma->data_addr2 = inp32(dma_base + DMA_DATA_ADDR2);
	s_dma->stride2 = inp32(dma_base + DMA_STRIDE2);
	s_dma->stretch_stride2 = inp32(dma_base + DMA_STRETCH_STRIDE2);
	s_dma->data_num2 = inp32(dma_base + DMA_DATA_NUM2);
}

void hisi_dss_chn_set_reg_default_value(struct hisi_fb_data_type *hisifd,
	char __iomem *dma_base)
{
	if (hisifd == NULL) {
		HISI_FB_DEBUG("hisifd is NULL!\n");
		return;
	}

	if (dma_base == NULL) {
		HISI_FB_DEBUG("dma_base is NULL!\n");
		return;
	}

	hisifd->set_reg(hisifd, dma_base + CH_REG_DEFAULT, 0x1, 32, 0);
	hisifd->set_reg(hisifd, dma_base + CH_REG_DEFAULT, 0x0, 32, 0);
}

static void hisi_dss_rdma_set_reg(struct hisi_fb_data_type *hisifd,
	char __iomem *dma_base, dss_rdma_t *s_dma)
{
	if (hisifd == NULL) {
		HISI_FB_DEBUG("hisifd is NULL!\n");
		return;
	}

	if (dma_base == NULL) {
		HISI_FB_DEBUG("dma_base is NULL!\n");
		return;
	}

	if (s_dma == NULL) {
		HISI_FB_DEBUG("s_dma is NULL!\n");
		return;
	}

	hisifd->set_reg(hisifd, dma_base + CH_REG_DEFAULT, 0x1, 32, 0);
	hisifd->set_reg(hisifd, dma_base + CH_REG_DEFAULT, 0x0, 32, 0);

	hisifd->set_reg(hisifd, dma_base + DMA_OFT_X0, s_dma->oft_x0, 32, 0);
	hisifd->set_reg(hisifd, dma_base + DMA_OFT_Y0, s_dma->oft_y0, 32, 0);
	hisifd->set_reg(hisifd, dma_base + DMA_OFT_X1, s_dma->oft_x1, 32, 0);
	hisifd->set_reg(hisifd, dma_base + DMA_OFT_Y1, s_dma->oft_y1, 32, 0);
	hisifd->set_reg(hisifd, dma_base + DMA_MASK0, s_dma->mask0, 32, 0);
	hisifd->set_reg(hisifd, dma_base + DMA_MASK1, s_dma->mask1, 32, 0);
	hisifd->set_reg(hisifd, dma_base + DMA_STRETCH_SIZE_VRT, s_dma->stretch_size_vrt, 32, 0);
	hisifd->set_reg(hisifd, dma_base + DMA_CTRL, s_dma->ctrl, 32, 0);
	hisifd->set_reg(hisifd, dma_base + DMA_TILE_SCRAM, s_dma->tile_scram, 32, 0);
	hisifd->set_reg(hisifd, dma_base + DMA_DATA_ADDR0, s_dma->data_addr0, 32 , 0);
	hisifd->set_reg(hisifd, dma_base + DMA_STRIDE0, s_dma->stride0, 32, 0);
	hisifd->set_reg(hisifd, dma_base + DMA_STRETCH_STRIDE0, s_dma->stretch_stride0, 32, 0);
	//hisifd->set_reg(hisifd, dma_base + DMA_DATA_NUM0, s_dma->data_num0, 32, 0);

	hisifd->set_reg(hisifd, dma_base + CH_RD_SHADOW, s_dma->ch_rd_shadow, 32, 0);
	hisifd->set_reg(hisifd, dma_base + CH_CTL, s_dma->ch_ctl, 32 , 0);

	if (s_dma->vpp_used) {
		hisifd->set_reg(hisifd, dma_base + VPP_CTRL, s_dma->vpp_ctrl, 32 , 0);
		//hisifd->set_reg(hisifd, dma_base + VPP_MEM_CTRL, s_dma->vpp_mem_ctrl, 32 , 0);
	}

	hisifd->set_reg(hisifd, dma_base + DMA_BUF_CTRL, s_dma->dma_buf_ctrl, 32, 0);

	if (s_dma->afbc_used) {
		hisifd->set_reg(hisifd, dma_base + AFBCD_HREG_HDR_PTR_LO, s_dma->afbcd_hreg_hdr_ptr_lo, 32 , 0);
		hisifd->set_reg(hisifd, dma_base + AFBCD_HREG_PIC_WIDTH, s_dma->afbcd_hreg_pic_width, 32 , 0);
		hisifd->set_reg(hisifd, dma_base + AFBCD_HREG_PIC_HEIGHT, s_dma->afbcd_hreg_pic_height, 32 , 0);
		hisifd->set_reg(hisifd, dma_base + AFBCD_HREG_FORMAT, s_dma->afbcd_hreg_format, 32 , 0);
		hisifd->set_reg(hisifd, dma_base + AFBCD_CTL, s_dma->afbcd_ctl, 32 , 0);
		hisifd->set_reg(hisifd, dma_base + AFBCD_STR, s_dma->afbcd_str, 32 , 0);
		hisifd->set_reg(hisifd, dma_base + AFBCD_LINE_CROP, s_dma->afbcd_line_crop, 32 , 0);
		hisifd->set_reg(hisifd, dma_base + AFBCD_INPUT_HEADER_STRIDE, s_dma->afbcd_input_header_stride, 32 , 0);
		hisifd->set_reg(hisifd, dma_base + AFBCD_PAYLOAD_STRIDE, s_dma->afbcd_payload_stride, 32 , 0);
		hisifd->set_reg(hisifd, dma_base + AFBCD_MM_BASE_0, s_dma->afbcd_mm_base_0, 32 , 0);
		hisifd->set_reg(hisifd, dma_base + AFBCD_AFBCD_PAYLOAD_POINTER, s_dma->afbcd_afbcd_payload_pointer, 32 , 0);
		hisifd->set_reg(hisifd, dma_base + AFBCD_HEIGHT_BF_STR, s_dma->afbcd_height_bf_str, 32 , 0);
		hisifd->set_reg(hisifd, dma_base + AFBCD_OS_CFG, s_dma->afbcd_os_cfg, 32 , 0);
		hisifd->set_reg(hisifd, dma_base + AFBCD_SCRAMBLE_MODE, s_dma->afbcd_scramble_mode, 32 , 0);
		hisifd->set_reg(hisifd, dma_base + AFBCD_HEADER_POINTER_OFFSET, s_dma->afbcd_header_pointer_offset, 32 , 0);
	}

}

static void hisi_dss_rdma_u_set_reg(struct hisi_fb_data_type *hisifd,
	char __iomem *dma_base, dss_rdma_t *s_dma)
{
	if (hisifd == NULL) {
		HISI_FB_DEBUG("hisifd is NULL!\n");
		return;
	}

	if (dma_base == NULL) {
		HISI_FB_DEBUG("dma_base is NULL!\n");
		return;
	}

	if (s_dma == NULL) {
		HISI_FB_DEBUG("s_dma is NULL!\n");
		return;
	}

	hisifd->set_reg(hisifd, dma_base + DMA_DATA_ADDR1, s_dma->data_addr1, 32 , 0);
	hisifd->set_reg(hisifd, dma_base + DMA_STRIDE1, s_dma->stride1, 32, 0);
	hisifd->set_reg(hisifd, dma_base + DMA_STRETCH_STRIDE1, s_dma->stretch_stride1, 32, 0);
	//hisifd->set_reg(hisifd, dma_base + DMA_DATA_NUM1, s_dma->data_num1, 32, 0);
}

static void hisi_dss_rdma_v_set_reg(struct hisi_fb_data_type *hisifd,
	char __iomem *dma_base, dss_rdma_t *s_dma)
{
	if (hisifd == NULL) {
		HISI_FB_DEBUG("hisifd is NULL!\n");
		return;
	}

	if (s_dma == NULL) {
		HISI_FB_DEBUG("s_dma is NULL!\n");
		return;
	}

	if (dma_base == NULL) {
		HISI_FB_DEBUG("dma_base is NULL!\n");
		return;
	}

	hisifd->set_reg(hisifd, dma_base + DMA_DATA_ADDR2, s_dma->data_addr2, 32 , 0);
	hisifd->set_reg(hisifd, dma_base + DMA_STRIDE2, s_dma->stride2, 32, 0);
	hisifd->set_reg(hisifd, dma_base + DMA_STRETCH_STRIDE2, s_dma->stretch_stride2, 32, 0);
	//hisifd->set_reg(hisifd, dma_base + DMA_DATA_NUM2, s_dma->data_num2, 32, 0);
}

static int hisi_get_rdma_tile_interleave(uint32_t stride)
{
	int i = 0;
	uint32_t interleave[MAX_TILE_SURPORT_NUM] = {256, 512, 1024, 2048, 4096, 8192};

	for (i = 0; i < MAX_TILE_SURPORT_NUM; i++) {
		if (interleave[i] == stride)
			return MIN_INTERLEAVE + i;
	}

	return 0;
}

static int hisi_dss_rdma_set_mmbuf_base_and_size(struct hisi_fb_data_type *hisifd,
	dss_layer_t *layer,int ovl_idx,dss_rect_ltrb_t *afbc_rect,uint32_t *mm_base_0,uint32_t *mm_base_1)
{
	bool mm_alloc_needed = false;
	int chn_idx = 0;
	dss_rect_t new_src_rect = {0,0,0,0};
	if(NULL == hisifd) {
		HISI_FB_ERR("hisifb is NULL");
		return -EINVAL;
	}

	if(NULL == afbc_rect) {
		HISI_FB_ERR("afbc_rect is NULL");
		return -EINVAL;
	}

	if(NULL == mm_base_0) {
		HISI_FB_ERR("mm_base_0 is NULL");
		return -EINVAL;
	}

	if(NULL == mm_base_1) {
		HISI_FB_ERR("mm_base_0 is NULL");
		return -EINVAL;
	}
	chn_idx = layer->chn_idx;
	new_src_rect = layer->src_rect;
	if ((layer->img.mmbuf_base > 0) && (layer->img.mmbuf_size > 0)) {
		*mm_base_0 = layer->img.mmbuf_base;
		*mm_base_1 = layer->img.mmbuf_base + layer->img.mmbuf_size / 2;
	} else {
		if (NULL == hisifd->mmbuf_info) {
			HISI_FB_ERR("hisifd->mmbuf_info is NULL");
			return -EINVAL;
		}

		if (ovl_idx <= DSS_OVL1) {
			mm_alloc_needed = true;
		} else {
			if (hisifd->mmbuf_info->mm_used[chn_idx] == 1)
				mm_alloc_needed = false;
			else
				mm_alloc_needed = true;
		}

		if (mm_alloc_needed) {
			afbc_rect->left = ALIGN_DOWN(new_src_rect.x, MMBUF_ADDR_ALIGN);
			afbc_rect->right = ALIGN_UP(new_src_rect.x - afbc_rect->left + new_src_rect.w, MMBUF_ADDR_ALIGN);

			hisifd->mmbuf_info->mm_size[chn_idx] = afbc_rect->right * layer->img.bpp * MMBUF_LINE_NUM;
			hisifd->mmbuf_info->mm_base[chn_idx] = hisi_dss_mmbuf_alloc(g_mmbuf_gen_pool,
				hisifd->mmbuf_info->mm_size[chn_idx]);
			if (hisifd->mmbuf_info->mm_base[chn_idx] < MMBUF_BASE) {
				HISI_FB_ERR("fb%d, chn%d failed to alloc mmbuf, mm_base=0x%x.\n",
					hisifd->index, chn_idx, hisifd->mmbuf_info->mm_base[chn_idx]);
				return -EINVAL;
			}
		}

		*mm_base_0 = hisifd->mmbuf_info->mm_base[chn_idx];
		*mm_base_1 = hisifd->mmbuf_info->mm_base[chn_idx] +
			hisifd->mmbuf_info->mm_size[chn_idx] / 2;
		hisifd->mmbuf_info->mm_used[chn_idx] = 1;
	}

	*mm_base_0 -= MMBUF_BASE;
	*mm_base_1 -= MMBUF_BASE;

	return 0;
}

static int hisi_dss_rdma_aligned_mask_rect(dss_layer_t *layer,dss_rect_ltrb_t *aligned_mask_rect,
	bool src_rect_mask_enable,int aligned_pixel) {
	if(NULL == layer) {
		HISI_FB_ERR("layer is NULL");
		return -EINVAL;
	}

	if(NULL == aligned_mask_rect) {
		HISI_FB_ERR("aligned_mask_rect is NULL");
		return -EINVAL;
	}

	if (src_rect_mask_enable) {
		if (is_YUV_P_420(layer->img.format) || is_YUV_P_422(layer->img.format)) {
			aligned_mask_rect->left = ALIGN_UP(layer->src_rect_mask.x, 2 * aligned_pixel);
			aligned_mask_rect->right = ALIGN_DOWN(layer->src_rect_mask.x + layer->src_rect_mask.w, 2 * aligned_pixel) - 1;
		} else {
			aligned_mask_rect->left = ALIGN_UP(layer->src_rect_mask.x, aligned_pixel);
			aligned_mask_rect->right = ALIGN_DOWN(layer->src_rect_mask.x + layer->src_rect_mask.w, aligned_pixel) - 1;
		}

		if (is_YUV_SP_420(layer->img.format) || is_YUV_P_420(layer->img.format)) {
			aligned_mask_rect->top = ALIGN_UP(layer->src_rect_mask.y, 2);
			aligned_mask_rect->bottom = ALIGN_DOWN(layer->src_rect_mask.y + layer->src_rect_mask.h, 2) - 1;
		} else {
			aligned_mask_rect->top = layer->src_rect_mask.y;
			aligned_mask_rect->bottom = DSS_HEIGHT(layer->src_rect_mask.y + layer->src_rect_mask.h);
		}
	}
	return 0;
}

static int  hisi_dss_rdma_aligned_rect(dss_layer_t *layer,dss_rect_ltrb_t *aligned_rect,
	dss_rect_t new_src_rect,int aligned_pixel) {

	if(NULL == layer) {
		HISI_FB_ERR("layer is NULL");
		return -EINVAL;
	}

	if(NULL == aligned_rect) {
		HISI_FB_ERR("aligned_rect is NULL");
		return -EINVAL;
	}

	if (is_YUV_P_420(layer->img.format) || is_YUV_P_422(layer->img.format)) {
		aligned_rect->left = ALIGN_DOWN(new_src_rect.x, 2 * aligned_pixel);
		aligned_rect->right = ALIGN_UP(new_src_rect.x + new_src_rect.w, 2 * aligned_pixel) - 1;
	} else {
		aligned_rect->left = ALIGN_DOWN(new_src_rect.x, aligned_pixel);
		aligned_rect->right = ALIGN_UP(new_src_rect.x + new_src_rect.w, aligned_pixel) - 1;
	}

	if (is_YUV_SP_420(layer->img.format) || is_YUV_P_420(layer->img.format)) {
		aligned_rect->top = ALIGN_DOWN(new_src_rect.y, 2);
		aligned_rect->bottom = ALIGN_UP(new_src_rect.y + new_src_rect.h, 2) - 1;
	} else {
		aligned_rect->top = new_src_rect.y;
		aligned_rect->bottom = DSS_HEIGHT(new_src_rect.y + new_src_rect.h);
	}

	return 0;
}

static int  hisi_dss_rdma_afbc_layer_aligned(dss_layer_t *layer,uint32_t mm_base_0,uint32_t mm_base_1)
{
	if(NULL == layer) {
		HISI_FB_ERR("layer is NULL");
		return -EINVAL;
	}

	if ((layer->img.width & (AFBC_HEADER_ADDR_ALIGN - 1)) ||
		(layer->img.height & (AFBC_BLOCK_ALIGN - 1))) {
		HISI_FB_ERR("layer%d img width(%d) is not %d bytes aligned, or "
			"img heigh(%d) is not %d bytes aligned!\n",
			layer->layer_idx, layer->img.width, AFBC_HEADER_ADDR_ALIGN,
			layer->img.height, AFBC_BLOCK_ALIGN);
		return -EINVAL;
	}

	if ((mm_base_0 & (MMBUF_ADDR_ALIGN - 1)) ||
		(mm_base_1 & (MMBUF_ADDR_ALIGN - 1)) ||
		(layer->img.mmbuf_size & (MMBUF_ADDR_ALIGN - 1))) {
		HISI_FB_ERR("layer%d mm_base_0(0x%x) is not %d bytes aligned, or "
			"mm_base_1(0x%x) is not %d bytes aligned, or mmbuf_size(0x%x) is "
			"not %d bytes aligned!\n",
			layer->layer_idx, mm_base_0, MMBUF_ADDR_ALIGN,
			mm_base_1, MMBUF_ADDR_ALIGN, layer->img.mmbuf_size, MMBUF_ADDR_ALIGN);
		return -EINVAL;
	}

	return 0;
}

static int hisi_dss_rdma_stretch(dss_layer_t *layer,dss_rect_t *out_aligned_rect,uint32_t *afbcd_half_block_mode,
	bool *rdma_stretch_enable,uint32_t *afbcd_stretch_inc,uint32_t *afbcd_stretch_acc) {
	if(NULL == layer)  {
		HISI_FB_ERR("layer is NULL");
		return -EINVAL;
	}
	if(NULL == out_aligned_rect || NULL == afbcd_half_block_mode ||
		NULL == rdma_stretch_enable || NULL == afbcd_stretch_inc || NULL == afbcd_stretch_acc)
	{
		HISI_FB_ERR("input data is NULL");
		return -EINVAL;
	}
	if (*rdma_stretch_enable) {
		*afbcd_stretch_inc = 0;
		*afbcd_stretch_acc = 0;

		//adjust out_aligned_rect
		out_aligned_rect->h /= 2;

		if (layer->transform & HISI_FB_TRANSFORM_FLIP_V) {
			*afbcd_half_block_mode = AFBC_HALF_BLOCK_LOWER_ONLY;
		} else {
			*afbcd_half_block_mode = AFBC_HALF_BLOCK_UPPER_ONLY;
		}
	} else {
		if (layer->transform & HISI_FB_TRANSFORM_FLIP_V) {
			*afbcd_half_block_mode = AFBC_HALF_BLOCK_LOWER_UPPER_ALL;
		} else {
			*afbcd_half_block_mode = AFBC_HALF_BLOCK_UPPER_LOWER_ALL;
		}
	}

	return 0;
}


typedef struct dss_rdma_oft_pos_type {
	int rdma_oft_x0;
	int rdma_oft_y0;
	int rdma_oft_x1;
	int rdma_oft_y1;
}dss_rdma_oft_pos;

typedef struct dss_rdma_mask_pos_type {
	int rdma_mask_x0;
	int rdma_mask_y0;
	int rdma_mask_x1;
	int rdma_mask_y1;
}dss_rdma_mask_pos;

static int hisi_dss_rdma_offset_pos_set(dss_layer_t *layer,dss_rdma_oft_pos *rdma_oft_pos,uint32_t *stretched_line_num)
{
	if(NULL == layer) {
		HISI_FB_ERR("layer is NULL");
		return -EINVAL;
	}
	if(NULL == rdma_oft_pos) {
		HISI_FB_ERR("rdma_oft_pos is NULL");
		return -EINVAL;
	}
	if(NULL == stretched_line_num) {
		HISI_FB_ERR("stretched_line_num is NULL");
		return -EINVAL;
	}

	if (is_YUV_P_420(layer->img.format) || is_YUV_P_422(layer->img.format)) {
		rdma_oft_pos->rdma_oft_x0 /= 2;
		rdma_oft_pos->rdma_oft_x1 = (rdma_oft_pos->rdma_oft_x1 + 1) / 2 - 1;
	}

	if (is_YUV_SP_420(layer->img.format) || is_YUV_P_420(layer->img.format)) {
		rdma_oft_pos->rdma_oft_y0 /= 2;
		rdma_oft_pos->rdma_oft_y1 = (rdma_oft_pos->rdma_oft_y1 + 1) / 2 - 1;

		*stretched_line_num /= 2;
	}

	return 0;
}


static int hisi_dss_rdma_set_dpp_and_dma(struct hisi_fb_data_type *hisifd,dss_layer_t *layer,int *aligned_pixel,
	bool *src_rect_mask_enable,dss_rdma_t *dma)
{
	int chn_idx = 0;
	int rdma_format = 0;
	int bpp = 0;
	bool is_pixel_10bit = false;
	bool is_yuv_semi_planar = false;
	bool is_yuv_planar = false;

	if(NULL == hisifd) {
		HISI_FB_ERR("hisifb is NULL");
		return -EINVAL;
	}
	if(NULL == layer) {
		HISI_FB_ERR("layer is NULL");
		return -EINVAL;
	}
	if(NULL == aligned_pixel ||NULL == src_rect_mask_enable) {
		HISI_FB_ERR("input data  is NULL");
		return -EINVAL;
	}
	chn_idx = layer->chn_idx;
	rdma_format = hisi_pixel_format_hal2dma(layer->img.format);
	if (rdma_format < 0) {
		HISI_FB_ERR("layer format(%d) not support !\n", layer->img.format);
		return -EINVAL;
	}
	is_pixel_10bit = isPixel10Bit2dma(rdma_format);
	is_yuv_semi_planar = isYUVSemiPlanar(layer->img.format);
	is_yuv_planar = isYUVPlanar(layer->img.format);

	bpp = (is_yuv_semi_planar || is_yuv_planar) ? 1 : layer->img.bpp;
	if (is_pixel_10bit) {
		bpp = layer->img.bpp;//lint !e713
	}
	*aligned_pixel = DMA_ALIGN_BYTES / (bpp);

	*src_rect_mask_enable = isSrcRectMasked(layer, *aligned_pixel);

	hisifd->dss_module.dma_used[chn_idx] = 1;

	if (layer->need_cap & CAP_YUV_DEINTERLACE) {
		dma->vpp_used = 1;

		if (layer->transform & HISI_FB_TRANSFORM_ROT_90) {
			dma->vpp_ctrl = set_bits32(dma->vpp_ctrl, 0x2, 2, 0);
		} else {
			dma->vpp_ctrl = set_bits32(dma->vpp_ctrl, 0x3, 2, 0);
		}
		//FIXME:
		//dma->vpp_mem_ctrl = set_bits32(dma->vpp_mem_ctrl , , , );
	}
	return 0;
}

static int hisi_dss_rdma_addr_aligned(dss_layer_t *layer,uint32_t *l2t_interleave_n,uint32_t *rdma_addr,bool mmu_enable)
{
	if(NULL == layer){
		HISI_FB_ERR("layer is NULL");
		return -EINVAL;
	}
	if(NULL == l2t_interleave_n){
		HISI_FB_ERR("l2t_interleave_n is NULL");
		return -EINVAL;
	}
	if(NULL == rdma_addr){
		HISI_FB_ERR("rdma_addr is NULL");
		return -EINVAL;
	}
	*rdma_addr = mmu_enable ? layer->img.vir_addr : layer->img.phy_addr;
	if (*rdma_addr & (DMA_ADDR_ALIGN - 1)) {
		HISI_FB_ERR("layer%d rdma_addr(0x%x) is not %d bytes aligned.\n",
			layer->layer_idx, *rdma_addr, DMA_ADDR_ALIGN);
		return -EINVAL;
	}

	if (layer->img.stride & (DMA_STRIDE_ALIGN - 1)) {
		HISI_FB_ERR("layer%d stride(0x%x) is not %d bytes aligned.\n",
			layer->layer_idx, layer->img.stride, DMA_STRIDE_ALIGN);
		return -EINVAL;
	}

	if (layer->need_cap & CAP_TILE) {
		*l2t_interleave_n = hisi_get_rdma_tile_interleave(layer->img.stride);
		if (*l2t_interleave_n < MIN_INTERLEAVE) {
			HISI_FB_ERR("tile stride should be 256*2^n, error stride:%d!\n", layer->img.stride);
			return -EINVAL;
		}

		if (*rdma_addr & (TILE_DMA_ADDR_ALIGN - 1)) {
			HISI_FB_ERR("layer%d tile rdma_addr(0x%x) is not %d bytes aligned.\n",
				layer->layer_idx, *rdma_addr, TILE_DMA_ADDR_ALIGN);
			return -EINVAL;
		}
	}
	return 0;
}

static int his_dss_rdma_afbcd_crop(dss_rect_ltrb_t *clip_rect,uint32_t *afbcd_top_crop_num,uint32_t *afbcd_bottom_crop_num)
{
	if(NULL == clip_rect){
		HISI_FB_ERR("clip_rect is NULL");
		return -EINVAL;
	}
	if(NULL == afbcd_top_crop_num ||NULL == afbcd_bottom_crop_num){
		HISI_FB_ERR("afbcd_crop_num is NULL");
		return -EINVAL;
	}
	*afbcd_top_crop_num = (clip_rect->top > AFBCD_TOP_CROP_MAX) ?
		AFBCD_TOP_CROP_MAX : clip_rect->top;
	*afbcd_bottom_crop_num = (clip_rect->bottom > AFBCD_BOTTOM_CROP_MAX) ?
		AFBCD_BOTTOM_CROP_MAX : clip_rect->bottom;

	clip_rect->top -= *afbcd_top_crop_num;
	if (clip_rect->top < 0) {
		HISI_FB_ERR("clip_rect->top is invalid");
		return -EINVAL;
	}
	clip_rect->bottom -= *afbcd_bottom_crop_num;
	if (clip_rect->bottom < 0) {
		HISI_FB_ERR("clip_rect->bottom is invalid");
		return -EINVAL;
	}
	return 0;
}

static int hisi_dss_rdma_oft_pos_check(dss_rdma_oft_pos rdma_oft_pos)
{
	if ((rdma_oft_pos.rdma_oft_x1 - rdma_oft_pos.rdma_oft_x0) < 0 ||
		(rdma_oft_pos.rdma_oft_x1 - rdma_oft_pos.rdma_oft_x0 + 1) > DMA_IN_WIDTH_MAX) {
		HISI_FB_ERR("out of range, rdma_oft_x0 = %d, rdma_oft_x1 = %d!\n",
			rdma_oft_pos.rdma_oft_x0, rdma_oft_pos.rdma_oft_x1);
		return -EINVAL;
	}

	if ((rdma_oft_pos.rdma_oft_y1 - rdma_oft_pos.rdma_oft_y0) < 0 ||
		(rdma_oft_pos.rdma_oft_y1 - rdma_oft_pos.rdma_oft_y0 + 1) > DMA_IN_HEIGHT_MAX) {
		HISI_FB_ERR("out of range, rdma_oft_y0 = %d, rdma_oft_y1 = %d\n",
			rdma_oft_pos.rdma_oft_y0, rdma_oft_pos.rdma_oft_y1);
		return -EINVAL;
	}
	return 0;
}

static int hisi_dss_rdma_mask_pos_set(dss_rdma_oft_pos rdma_oft_pos,dss_rdma_mask_pos *rdma_mask_pos,
	dss_rect_ltrb_t aligned_mask_rect,bool *src_rect_mask_enable,int aligned_pixel)
{
	if(NULL == rdma_mask_pos){
		HISI_FB_ERR("rdma_mask_pos is NULL");
		return -EINVAL;
	}
	if(NULL == src_rect_mask_enable){
		HISI_FB_ERR("src_rect_mask_enable is NULL");
		return -EINVAL;
	}
	if (*src_rect_mask_enable) {
		rdma_mask_pos->rdma_mask_y0 = aligned_mask_rect.top;
		rdma_mask_pos->rdma_mask_y1 = aligned_mask_rect.bottom;
		rdma_mask_pos->rdma_mask_x0 = aligned_mask_rect.left / aligned_pixel;
		rdma_mask_pos->rdma_mask_x1 = aligned_mask_rect.right / aligned_pixel;

		//Fix bug
		if ((rdma_mask_pos->rdma_mask_x1 - rdma_mask_pos->rdma_mask_x0) > 2)
			rdma_mask_pos->rdma_mask_x0 += 2;

		if ((rdma_mask_pos->rdma_mask_x0 <= rdma_oft_pos.rdma_oft_x0) || (rdma_mask_pos->rdma_mask_x1 >= rdma_oft_pos.rdma_oft_x1)
			|| (rdma_mask_pos->rdma_mask_y0 <= rdma_oft_pos.rdma_oft_y0) || (rdma_mask_pos->rdma_mask_y1 >= rdma_oft_pos.rdma_oft_y1)) {
			 *src_rect_mask_enable = false;
			 rdma_mask_pos->rdma_mask_x0 = 0;
			 rdma_mask_pos->rdma_mask_y0 = 0;
			 rdma_mask_pos->rdma_mask_x1 = 0;
			 rdma_mask_pos->rdma_mask_y1 = 0;
		}
	}
	return 0;
}

static int hisi_dss_rdma_payload_set(dss_layer_t *layer,uint32_t *afbc_payload_stride,uint32_t *afbc_payload_addr,dss_rect_ltrb_t aligned_rect)
{
	uint32_t stride_align = 0;
	uint32_t addr_align = 0;
	if(NULL == layer){
		HISI_FB_ERR("layer is NULL");
		return -EINVAL;
	}
	if(NULL == afbc_payload_stride || NULL == afbc_payload_addr){
		HISI_FB_ERR("afbc_payload is NULL");
		return -EINVAL;
	}
	if (layer->img.bpp == 4) {
		stride_align = AFBC_PAYLOAD_STRIDE_ALIGN_32;
		addr_align = AFBC_PAYLOAD_ADDR_ALIGN_32;
	} else if (layer->img.bpp == 2) {
		stride_align = AFBC_PAYLOAD_STRIDE_ALIGN_16;
		addr_align = AFBC_PAYLOAD_ADDR_ALIGN_16;
	} else {
		HISI_FB_ERR("bpp(%d) not supported!\n", layer->img.bpp);
		return -EINVAL;
	}

	*afbc_payload_stride = layer->img.afbc_payload_stride;
	if (layer->img.afbc_scramble_mode != DSS_AFBC_SCRAMBLE_MODE2) {
		*afbc_payload_stride = (layer->img.width / AFBC_BLOCK_ALIGN) * stride_align;
	}
	*afbc_payload_addr = layer->img.afbc_payload_addr +
		(aligned_rect.top / AFBC_BLOCK_ALIGN) * (*afbc_payload_stride) +
		(aligned_rect.left / AFBC_BLOCK_ALIGN) * stride_align;

	if ((*afbc_payload_addr & (addr_align - 1)) ||
		(*afbc_payload_stride & (stride_align - 1))) {
		HISI_FB_ERR("layer%d afbc_payload_addr(0x%x) is not %d bytes aligned, or "
			"afbc_payload_stride(0x%x) is not %d bytes aligned!\n",
			layer->layer_idx, *afbc_payload_addr, addr_align,
			*afbc_payload_stride, stride_align);
		return -EINVAL;
	}
	return 0;
}

static int hisi_dss_rdma_head_check(dss_layer_t *layer,uint32_t afbc_header_addr,uint32_t afbc_header_stride,
	dss_rect_ltrb_t aligned_rect,dss_rect_ltrb_t afbc_rect,uint32_t *afbc_header_start_pos)
{
	if(NULL == layer){
		HISI_FB_ERR("layer is NULL");
		return -EINVAL;
	}
	if(NULL == afbc_header_start_pos){
		HISI_FB_ERR("afbc_header_start_pos is NULL");
		return -EINVAL;
	}
	if ((afbc_header_addr & (AFBC_HEADER_ADDR_ALIGN - 1)) ||
		(afbc_header_stride & (AFBC_HEADER_STRIDE_ALIGN - 1))) {
		HISI_FB_ERR("layer%d afbc_header_addr(0x%x) is not %d bytes aligned, or "
			"afbc_header_stride(0x%x) is not %d bytes aligned!\n",
			layer->layer_idx, afbc_header_addr, AFBC_HEADER_ADDR_ALIGN,
			afbc_header_stride, AFBC_HEADER_STRIDE_ALIGN);
		return -EINVAL;
	}

	if ((aligned_rect.left - afbc_rect.left) < 0) {
		HISI_FB_ERR("aligned_rect.left(%d) small than  afbc_rect.left(%d) is err.\n",
			aligned_rect.left, afbc_rect.left);
		return -EINVAL;
	};

	*afbc_header_start_pos = (aligned_rect.left - afbc_rect.left) / AFBC_BLOCK_ALIGN;
	return 0;
}

int hisi_dss_rdma_config(struct hisi_fb_data_type *hisifd, int ovl_idx,
	dss_layer_t *layer, dss_rect_ltrb_t *clip_rect,
	dss_rect_t *out_aligned_rect, bool *rdma_stretch_enable)
{
	dss_rdma_t *dma = NULL;

	bool mmu_enable = false;
	bool is_yuv_semi_planar = false;
	bool is_yuv_planar = false;
	bool src_rect_mask_enable = false;

	uint32_t rdma_addr = 0;
	uint32_t rdma_stride = 0;
	int rdma_format = 0;
	int rdma_transform = 0;
	int rdma_data_num = 0;
	uint32_t stretch_size_vrt = 0;
	uint32_t stretched_line_num = 0;
	uint32_t stretched_stride = 0;

	int aligned_pixel = 0;
	dss_rdma_oft_pos rdma_oft_pos = {0,0,0,0};
	dss_rdma_mask_pos rdma_mask_pos = {0,0,0,0};


	int chn_idx = 0;
	uint32_t l2t_interleave_n = 0;

	dss_rect_ltrb_t aligned_rect = {0, 0, 0, 0};
	dss_rect_ltrb_t aligned_mask_rect = {0, 0, 0, 0};
	dss_rect_t new_src_rect;

	bool is_pixel_10bit = false;
	uint32_t afbcd_half_block_mode = 0;
	uint32_t afbcd_stretch_acc = 0;
	uint32_t afbcd_stretch_inc = 0;
	uint32_t afbcd_height_bf_str = 0;
	uint32_t afbcd_top_crop_num = 0;
	uint32_t afbcd_bottom_crop_num = 0;
	uint32_t afbc_header_addr = 0;
	uint32_t afbc_header_stride = 0;
	uint32_t afbc_payload_addr = 0;
	uint32_t afbc_payload_stride = 0;
	uint32_t afbc_header_start_pos = 0;
	uint32_t afbc_header_pointer_offset = 0;
	dss_rect_ltrb_t afbc_rect;
	uint32_t mm_base_0 = 0;
	uint32_t mm_base_1 = 0;
	int ret = 0;

	if (NULL == hisifd) {
		HISI_FB_ERR("hisifd is NULL");
		return -EINVAL;
	}
	if (NULL == layer) {
		HISI_FB_ERR("layer is NULL");
		return -EINVAL;
	}
	if ((ovl_idx < DSS_OVL0) || (ovl_idx >= DSS_OVL_IDX_MAX)) {
		HISI_FB_ERR("ovl_idx is invalid");
		return -EINVAL;
	}

	chn_idx = layer->chn_idx;
	new_src_rect = layer->src_rect;

	stretched_line_num = isNeedRdmaStretchBlt(hisifd, layer);
	*rdma_stretch_enable = (stretched_line_num > 0) ? true : false;

	mmu_enable = (layer->img.mmu_enable == 1) ? true : false;
	is_yuv_semi_planar = isYUVSemiPlanar(layer->img.format);
	is_yuv_planar = isYUVPlanar(layer->img.format);

	rdma_format = hisi_pixel_format_hal2dma(layer->img.format);
	if (rdma_format < 0) {
		HISI_FB_ERR("layer format(%d) not support !\n", layer->img.format);
		return -EINVAL;
	}

	rdma_transform = hisi_transform_hal2dma(layer->transform, chn_idx);
	if (rdma_transform < 0) {
		HISI_FB_ERR("layer transform(%d) not support!\n", layer->transform);
		return -EINVAL;
	}

	is_pixel_10bit = isPixel10Bit2dma(rdma_format);

	dma = &(hisifd->dss_module.rdma[chn_idx]);
	ret = hisi_dss_rdma_set_dpp_and_dma(hisifd,layer,&aligned_pixel,&src_rect_mask_enable,dma);
	if(ret != 0) {
		return -EINVAL;
	}

	if (layer->need_cap & CAP_HFBCD) {
		return 0;
	}

	if (layer->need_cap & CAP_AFBCD) {
		ret = hisi_dss_rdma_set_mmbuf_base_and_size(hisifd,layer,ovl_idx,&afbc_rect,&mm_base_0,&mm_base_1);
		if (ret != 0) {
			return -EINVAL;
		}

		ret = hisi_dss_rdma_afbc_layer_aligned(layer,mm_base_0,mm_base_1);
		if (ret != 0) {
			return -EINVAL;
		}

		dma->afbc_used = 1;

		//aligned rect
		aligned_rect.left = ALIGN_DOWN(new_src_rect.x, AFBC_BLOCK_ALIGN);
		aligned_rect.right = ALIGN_UP(new_src_rect.x + new_src_rect.w, AFBC_BLOCK_ALIGN) - 1;
		aligned_rect.top = ALIGN_DOWN(new_src_rect.y, AFBC_BLOCK_ALIGN);
		aligned_rect.bottom = ALIGN_UP(new_src_rect.y + new_src_rect.h, AFBC_BLOCK_ALIGN) - 1;

		//out_aligned_rect
		out_aligned_rect->x = 0;
		out_aligned_rect->y = 0;
		out_aligned_rect->w = aligned_rect.right - aligned_rect.left + 1;
		out_aligned_rect->h = aligned_rect.bottom - aligned_rect.top + 1;

		afbcd_height_bf_str = aligned_rect.bottom - aligned_rect.top + 1;

		//stretch
		ret = hisi_dss_rdma_stretch(layer,out_aligned_rect,&afbcd_half_block_mode,
			rdma_stretch_enable,&afbcd_stretch_inc,&afbcd_stretch_acc);
		if (ret != 0) {
			return -EINVAL;
		}
		if (layer->img.afbc_header_addr & (AFBC_SUPER_GRAPH_HEADER_ADDR_ALIGN - 1)) {
			HISI_FB_ERR("layer%d super graph afbc_header_addr(0x%x) is not %d bytes aligned!\n",
				layer->layer_idx, afbc_header_addr, AFBC_SUPER_GRAPH_HEADER_ADDR_ALIGN);
			return -EINVAL;
		}
		// rdfc clip_rect
		clip_rect->left = new_src_rect.x - aligned_rect.left;
		clip_rect->right = aligned_rect.right - DSS_WIDTH(new_src_rect.x + new_src_rect.w);
		clip_rect->top = new_src_rect.y - aligned_rect.top;
		clip_rect->bottom = aligned_rect.bottom - DSS_HEIGHT(new_src_rect.y + new_src_rect.h);
		if (hisi_adjust_clip_rect(layer, clip_rect) < 0) {
			HISI_FB_ERR("clip rect invalid => layer_idx=%d, chn_idx=%d, clip_rect(%d, %d, %d, %d).\n",
				layer->layer_idx, chn_idx, clip_rect->left, clip_rect->right,
				clip_rect->top, clip_rect->bottom);
			return -EINVAL;
		}

		//afbcd crop
		ret = his_dss_rdma_afbcd_crop(clip_rect,&afbcd_top_crop_num,&afbcd_bottom_crop_num);
		if(ret != 0){
			return -EINVAL;
		}
		//adjust out_aligned_rect
		out_aligned_rect->h -= (afbcd_top_crop_num + afbcd_bottom_crop_num);

		rdma_oft_pos.rdma_oft_x0 = aligned_rect.left / aligned_pixel;
		rdma_oft_pos.rdma_oft_x1 = aligned_rect.right / aligned_pixel;
		stretch_size_vrt = DSS_HEIGHT(out_aligned_rect->h);
		stretched_line_num = 0;

		// afbc rect
		afbc_rect = aligned_rect;
		afbc_rect.left = ALIGN_DOWN(new_src_rect.x, AFBC_HEADER_ADDR_ALIGN);
		afbc_rect.right = ALIGN_UP(new_src_rect.x + new_src_rect.w, AFBC_HEADER_ADDR_ALIGN) - 1;

		//header
		afbc_header_stride = (layer->img.width / AFBC_BLOCK_ALIGN) * AFBC_HEADER_STRIDE_BLOCK;
		afbc_header_pointer_offset = (afbc_rect.top / AFBC_BLOCK_ALIGN) * afbc_header_stride +
			(afbc_rect.left / AFBC_BLOCK_ALIGN) * AFBC_HEADER_STRIDE_BLOCK;
		afbc_header_addr = layer->img.afbc_header_addr + afbc_header_pointer_offset;
		ret = hisi_dss_rdma_head_check(layer,afbc_header_addr,afbc_header_stride,
			aligned_rect,afbc_rect,&afbc_header_start_pos);
		if(ret != 0){
			return -EINVAL;
		}

		//payload
		ret = hisi_dss_rdma_payload_set(layer,&afbc_payload_stride,&afbc_payload_addr,aligned_rect);
		if(ret != 0){
			return -EINVAL;
		}
		if (g_debug_ovl_online_composer || g_debug_ovl_offline_composer || g_debug_ovl_copybit_composer) {
			HISI_FB_INFO("fb%d, mm_base_0=0x%x, mm_base_1=0x%x, mmbuf_size=%d, "
				"aligned_rect(%d,%d,%d,%d), afbc_rect(%d,%d,%d,%d)!\n",
				hisifd->index, mm_base_0, mm_base_1, layer->img.mmbuf_size,
				aligned_rect.left, aligned_rect.top, aligned_rect.right, aligned_rect.bottom,
				afbc_rect.left, afbc_rect.top, afbc_rect.right, afbc_rect.bottom);
		}

		dma->oft_x0 = set_bits32(dma->oft_x0, rdma_oft_pos.rdma_oft_x0, 16, 0);
		dma->oft_x1 = set_bits32(dma->oft_x1, rdma_oft_pos.rdma_oft_x1, 16, 0);
		dma->stretch_size_vrt = set_bits32(dma->stretch_size_vrt,
			(stretch_size_vrt | (stretched_line_num << 13)), 19, 0);
		dma->ctrl = set_bits32(dma->ctrl, rdma_format, 5, 3);
		dma->ctrl = set_bits32(dma->ctrl, (mmu_enable ? 0x1 : 0x0), 1, 8);
		dma->ctrl = set_bits32(dma->ctrl, rdma_transform, 3, 9);
		dma->ctrl = set_bits32(dma->ctrl, (*rdma_stretch_enable ? 1 : 0), 1, 12);
		dma->ch_ctl = set_bits32(dma->ch_ctl, 0x1, 1, 0);
		dma->ch_ctl = set_bits32(dma->ch_ctl, 0x1, 1, 2);

		dma->afbcd_hreg_pic_width = set_bits32(dma->afbcd_hreg_pic_width,
			(aligned_rect.right - aligned_rect.left), 16, 0);
		dma->afbcd_hreg_pic_height = set_bits32(dma->afbcd_hreg_pic_height,
			(aligned_rect.bottom - aligned_rect.top), 16, 0);
		//block split mode
		dma->afbcd_hreg_format = set_bits32(dma->afbcd_hreg_format, 0x1, 1, 0);
		//color transform
		dma->afbcd_hreg_format = set_bits32(dma->afbcd_hreg_format,
			(isYUVPackage(layer->img.format) ? 0x0 : 0x1), 1, 21);
		dma->afbcd_ctl = set_bits32(dma->afbcd_ctl, afbcd_half_block_mode, 2, 6);
		dma->afbcd_str = set_bits32(dma->afbcd_str,
			(afbcd_stretch_acc << 8 | afbcd_stretch_inc), 12, 0);

		dma->afbcd_line_crop = set_bits32(dma->afbcd_line_crop,
			(afbcd_top_crop_num << 4 | afbcd_bottom_crop_num), 8, 0);

		dma->afbcd_hreg_hdr_ptr_lo = set_bits32(dma->afbcd_hreg_hdr_ptr_lo,
			afbc_header_addr, 32, 0);
		dma->afbcd_input_header_stride = set_bits32(dma->afbcd_input_header_stride,
			(afbc_header_start_pos << 14) | afbc_header_stride, 16, 0);
		dma->afbcd_payload_stride = set_bits32(dma->afbcd_payload_stride,
			afbc_payload_stride, 20, 0);
		dma->afbcd_mm_base_0 = set_bits32(dma->afbcd_mm_base_0, mm_base_0, 32, 0);
		dma->afbcd_afbcd_payload_pointer = set_bits32(dma->afbcd_afbcd_payload_pointer,
			afbc_payload_addr, 32, 0);
		dma->afbcd_height_bf_str = set_bits32(dma->afbcd_height_bf_str,
			DSS_HEIGHT(afbcd_height_bf_str), 16, 0);
		//dma->afbcd_os_cfg = set_bits32(dma->afbcd_os_cfg, , , );
		//dma->afbcd_mem_ctrl = set_bits32(dma->afbcd_mem_ctrl, , , );
		dma->afbcd_header_pointer_offset = set_bits32(dma->afbcd_header_pointer_offset,
			afbc_header_pointer_offset, 32, 0);
		//afbcd_scramble_mode
		dma->afbcd_scramble_mode = set_bits32(dma->afbcd_scramble_mode,
			layer->img.afbc_scramble_mode, 2, 0);

		return 0;
	}

	ret = hisi_dss_rdma_addr_aligned(layer,&l2t_interleave_n,&rdma_addr,mmu_enable);
	if(ret != 0){
		HISI_FB_ERR("rdma_addr is not aligned");
		return -EINVAL;
	}

	// aligned_rect
	ret = hisi_dss_rdma_aligned_rect(layer,&aligned_rect,new_src_rect,aligned_pixel);
	if(ret != 0) {
		return -EINVAL;
	}
	// aligned_mask_rect
	ret = hisi_dss_rdma_aligned_mask_rect(layer,&aligned_mask_rect,src_rect_mask_enable,aligned_pixel);
	if(ret != 0) {
		return -EINVAL;
	}
	// out_rect
	out_aligned_rect->x = 0;
	out_aligned_rect->y = 0;
	out_aligned_rect->w = aligned_rect.right - aligned_rect.left + 1;
	out_aligned_rect->h = aligned_rect.bottom - aligned_rect.top + 1;
	if (stretched_line_num > 0) {
		stretch_size_vrt = (out_aligned_rect->h / stretched_line_num) +
			((out_aligned_rect->h % stretched_line_num) ? 1 : 0) - 1;

		out_aligned_rect->h = stretch_size_vrt + 1;
	} else {
		stretch_size_vrt = 0x0;
	}

	// clip_rect
	clip_rect->left = new_src_rect.x - aligned_rect.left;
	clip_rect->right = aligned_rect.right - DSS_WIDTH(new_src_rect.x + new_src_rect.w);
	clip_rect->top = new_src_rect.y - aligned_rect.top;
	clip_rect->bottom = aligned_rect.bottom - DSS_HEIGHT(new_src_rect.y + new_src_rect.h);

	if (hisi_adjust_clip_rect(layer, clip_rect) < 0) {
		HISI_FB_ERR("clip rect invalid => layer_idx=%d, chn_idx=%d, clip_rect(%d, %d, %d, %d).\n",
			layer->layer_idx, chn_idx, clip_rect->left, clip_rect->right,
			clip_rect->top, clip_rect->bottom);
		return -EINVAL;
	}

	rdma_oft_pos.rdma_oft_y0 = aligned_rect.top;
	rdma_oft_pos.rdma_oft_y1 = aligned_rect.bottom;
	rdma_oft_pos.rdma_oft_x0 = aligned_rect.left / aligned_pixel;
	rdma_oft_pos.rdma_oft_x1 = aligned_rect.right / aligned_pixel;

	ret = hisi_dss_rdma_oft_pos_check(rdma_oft_pos);
	if(ret != 0){
		return -EINVAL;
	}

	rdma_addr = hisi_calculate_display_addr(mmu_enable, layer, &aligned_rect, DSS_ADDR_PLANE0, is_pixel_10bit);
	rdma_stride = layer->img.stride;
	rdma_data_num = (rdma_oft_pos.rdma_oft_x1 - rdma_oft_pos.rdma_oft_x0 + 1) * (rdma_oft_pos.rdma_oft_y1- rdma_oft_pos.rdma_oft_y0 + 1);
	ret = hisi_dss_rdma_mask_pos_set(rdma_oft_pos,&rdma_mask_pos,aligned_mask_rect,&src_rect_mask_enable,aligned_pixel);
	if(ret != 0){
		return -EINVAL;
	}
	if (stretched_line_num > 0) {
		stretched_stride = stretched_line_num * rdma_stride / DMA_ALIGN_BYTES;
		rdma_data_num = (stretch_size_vrt + 1) * (rdma_oft_pos.rdma_oft_x1 - rdma_oft_pos.rdma_oft_x0 + 1);
	} else {
		stretch_size_vrt = rdma_oft_pos.rdma_oft_y1 - rdma_oft_pos.rdma_oft_y0;
		stretched_line_num = 0x0;
		stretched_stride = 0x0;
	}

	dma->oft_x0 = set_bits32(dma->oft_x0, rdma_oft_pos.rdma_oft_x0, 16, 0);
	dma->oft_y0 = set_bits32(dma->oft_y0, rdma_oft_pos.rdma_oft_y0, 16, 0);
	dma->oft_x1 = set_bits32(dma->oft_x1, rdma_oft_pos.rdma_oft_x1, 16, 0);
	dma->oft_y1 = set_bits32(dma->oft_y1, rdma_oft_pos.rdma_oft_y1, 16, 0);
	dma->mask0 = set_bits32(dma->mask0,
		(rdma_mask_pos.rdma_mask_y0 | (rdma_mask_pos.rdma_mask_x0 << 16)), 32, 0);
	dma->mask1 = set_bits32(dma->mask1,
		(rdma_mask_pos.rdma_mask_y1 | (rdma_mask_pos.rdma_mask_x1 << 16)), 32, 0);
	dma->stretch_size_vrt = set_bits32(dma->stretch_size_vrt,
		(stretch_size_vrt | (stretched_line_num << 13)), 19, 0);
	dma->ctrl = set_bits32(dma->ctrl, ((layer->need_cap & CAP_TILE) ? 0x1 : 0x0), 1, 1);
	dma->ctrl = set_bits32(dma->ctrl, rdma_format, 5, 3);
	dma->ctrl = set_bits32(dma->ctrl, (mmu_enable ? 0x1 : 0x0), 1, 8);
	dma->ctrl = set_bits32(dma->ctrl, rdma_transform, 3, 9);
	dma->ctrl = set_bits32(dma->ctrl, ((stretched_line_num > 0) ? 0x1 : 0x0), 1, 12);
	dma->ctrl = set_bits32(dma->ctrl, (src_rect_mask_enable ? 0x1 : 0x0), 1, 17);
	dma->tile_scram = set_bits32(dma->tile_scram, ((layer->need_cap & CAP_TILE) ? 0x1 : 0x0), 1, 0);
	dma->ch_ctl = set_bits32(dma->ch_ctl, 0x1, 1, 0);

	dma->data_addr0 = set_bits32(dma->data_addr0, rdma_addr, 32, 0);
	dma->stride0 = set_bits32(dma->stride0,
		((rdma_stride / DMA_ALIGN_BYTES) | (l2t_interleave_n << 16)), 20, 0);
	dma->stretch_stride0 = set_bits32(dma->stretch_stride0, stretched_stride, 19, 0);
	dma->data_num0 = set_bits32(dma->data_num0, rdma_data_num, 30, 0);


	if (is_yuv_semi_planar || is_yuv_planar) {
		ret = hisi_dss_rdma_offset_pos_set(layer,&rdma_oft_pos,&stretched_line_num);
		if(ret !=  0) {
			return -EINVAL;
		}

		rdma_addr = hisi_calculate_display_addr(mmu_enable, layer, &aligned_rect, DSS_ADDR_PLANE1, is_pixel_10bit);
		rdma_stride = layer->img.stride_plane1;
		rdma_data_num = (rdma_oft_pos.rdma_oft_x1 - rdma_oft_pos.rdma_oft_x0 + 1) * (rdma_oft_pos.rdma_oft_y1- rdma_oft_pos.rdma_oft_y0 + 1) * 2;

		if (*rdma_stretch_enable) {
			stretched_stride = stretched_line_num * rdma_stride / DMA_ALIGN_BYTES;
			rdma_data_num = (stretch_size_vrt + 1) * (rdma_oft_pos.rdma_oft_x1 - rdma_oft_pos.rdma_oft_x0 + 1) * 2;
		} else {
			stretch_size_vrt = 0;
			stretched_line_num = 0;
			stretched_stride = 0;
		}

		dma->data_addr1 = set_bits32(dma->data_addr1, rdma_addr, 32, 0);
		dma->stride1 = set_bits32(dma->stride1,
			((rdma_stride / DMA_ALIGN_BYTES) | (l2t_interleave_n << 16)), 20, 0);
		dma->stretch_stride1 = set_bits32(dma->stretch_stride1, stretched_stride, 19, 0);
		dma->data_num1 = set_bits32(dma->data_num1, rdma_data_num, 30, 0);

		if (is_yuv_planar) {
			rdma_addr = hisi_calculate_display_addr(mmu_enable, layer, &aligned_rect, DSS_ADDR_PLANE2, is_pixel_10bit);
			rdma_stride = layer->img.stride_plane2;

			dma->data_addr2 = set_bits32(dma->data_addr2, rdma_addr, 32, 0);
			dma->stride2 = set_bits32(dma->stride2,
				((rdma_stride / DMA_ALIGN_BYTES) | (l2t_interleave_n << 16)), 20, 0);
			dma->stretch_stride2 = set_bits32(dma->stretch_stride1, stretched_stride, 19, 0);
			dma->data_num2 = set_bits32(dma->data_num1, rdma_data_num, 30, 0);
		}
	}

	return ret;
}


/*******************************************************************************
** DSS DFC
*/
void hisi_dss_dfc_init(char __iomem *dfc_base, dss_dfc_t *s_dfc)
{
	if (NULL == dfc_base) {
		HISI_FB_ERR("dfc_base is NULL");
		return;
	}
	if (NULL == s_dfc) {
		HISI_FB_ERR("s_dfc is NULL");
		return;
	}

	memset(s_dfc, 0, sizeof(dss_dfc_t));

	s_dfc->disp_size = inp32(dfc_base + DFC_DISP_SIZE);
	s_dfc->pix_in_num = inp32(dfc_base + DFC_PIX_IN_NUM);
	s_dfc->disp_fmt = inp32(dfc_base + DFC_DISP_FMT);
	s_dfc->clip_ctl_hrz = inp32(dfc_base + DFC_CLIP_CTL_HRZ);
	s_dfc->clip_ctl_vrz = inp32(dfc_base + DFC_CLIP_CTL_VRZ);
	s_dfc->ctl_clip_en = inp32(dfc_base + DFC_CTL_CLIP_EN);
	s_dfc->icg_module = inp32(dfc_base + DFC_ICG_MODULE);
	s_dfc->dither_enable = inp32(dfc_base + DFC_DITHER_ENABLE);
	s_dfc->padding_ctl = inp32(dfc_base + DFC_PADDING_CTL);
}

static void hisi_dss_dfc_set_reg(struct hisi_fb_data_type *hisifd,
	char __iomem *dfc_base, dss_dfc_t *s_dfc)
{
	if (hisifd == NULL) {
		HISI_FB_DEBUG("hisifd is NULL!\n");
		return;
	}

	if (dfc_base == NULL) {
		HISI_FB_DEBUG("dfc_base is NULL!\n");
		return;
	}

	if (s_dfc == NULL) {
		HISI_FB_DEBUG("s_dfc is NULL!\n");
		return;
	}

	hisifd->set_reg(hisifd, dfc_base + DFC_DISP_SIZE, s_dfc->disp_size, 32, 0);
	hisifd->set_reg(hisifd, dfc_base + DFC_PIX_IN_NUM, s_dfc->pix_in_num, 32, 0);
	hisifd->set_reg(hisifd, dfc_base + DFC_DISP_FMT, s_dfc->disp_fmt, 32, 0);
	hisifd->set_reg(hisifd, dfc_base + DFC_CLIP_CTL_HRZ, s_dfc->clip_ctl_hrz, 32, 0);
	hisifd->set_reg(hisifd, dfc_base + DFC_CLIP_CTL_VRZ, s_dfc->clip_ctl_vrz, 32, 0);
	hisifd->set_reg(hisifd, dfc_base + DFC_CTL_CLIP_EN, s_dfc->ctl_clip_en, 32, 0);
	hisifd->set_reg(hisifd, dfc_base + DFC_ICG_MODULE, s_dfc->icg_module, 32, 0);
	hisifd->set_reg(hisifd, dfc_base + DFC_DITHER_ENABLE, s_dfc->dither_enable, 32, 0);
	hisifd->set_reg(hisifd, dfc_base + DFC_PADDING_CTL, s_dfc->padding_ctl, 32, 0);

}

int hisi_dss_rdfc_config(struct hisi_fb_data_type *hisifd, dss_layer_t *layer,
	dss_rect_t *aligned_rect, dss_rect_ltrb_t clip_rect)
{
	dss_dfc_t *dfc = NULL;
	int chn_idx = 0;
	int dfc_fmt = 0;
	int dfc_bpp = 0;
	int dfc_pix_in_num = 0;
	int dfc_aligned = 0;
	int size_hrz = 0;
	int size_vrt = 0;
	int dfc_hrz_clip = 0;
	bool need_clip = false;
	bool is_pixel_10bit = false;

	if (NULL == hisifd) {
		HISI_FB_ERR("hisifd is NULL");
		return -EINVAL;
	}
	if (NULL == layer) {
		HISI_FB_ERR("layer is NULL");
		return -EINVAL;
	}

	chn_idx = layer->chn_idx;

	dfc = &(hisifd->dss_module.dfc[chn_idx]);
	hisifd->dss_module.dfc_used[chn_idx] = 1;

	dfc_fmt = hisi_pixel_format_hal2dfc(layer->img.format);
	if (dfc_fmt < 0) {
		HISI_FB_ERR("layer format (%d) not support !\n", layer->img.format);
		return -EINVAL;
	}

	is_pixel_10bit = isPixel10Bit2dfc(dfc_fmt);

	dfc_bpp = hisi_dfc_get_bpp(dfc_fmt);
	if (dfc_bpp <= 0) {
		HISI_FB_ERR("dfc_bpp(%d) not support !\n", dfc_bpp);
		return -EINVAL;
	}

	dfc_pix_in_num = (dfc_bpp <= 2) ? 0x1 : 0x0;
	dfc_aligned = (dfc_bpp <= 2) ? 4 : 2;

	need_clip = isNeedRectClip(clip_rect);

	size_hrz = DSS_WIDTH(aligned_rect->w);
	size_vrt = DSS_HEIGHT(aligned_rect->h);

	if (((size_hrz + 1) % dfc_aligned) != 0) {
		size_hrz -= 1;
		HISI_FB_ERR("SIZE_HRT=%d mismatch!bpp=%d\n", size_hrz, layer->img.bpp);

		HISI_FB_ERR("layer_idx%d, format=%d, transform=%d, "
			"original_src_rect(%d,%d,%d,%d), rdma_out_rect(%d,%d,%d,%d), dst_rect(%d,%d,%d,%d)!\n",
			layer->layer_idx, layer->img.format, layer->transform,
			layer->src_rect.x, layer->src_rect.y, layer->src_rect.w, layer->src_rect.h,
			aligned_rect->x, aligned_rect->y, aligned_rect->w, aligned_rect->h,
			layer->dst_rect.x, layer->dst_rect.y, layer->dst_rect.w, layer->dst_rect.h);
	}

	dfc_hrz_clip = (size_hrz + 1) % dfc_aligned;
	if (dfc_hrz_clip) {
		clip_rect.right += dfc_hrz_clip;
		size_hrz += dfc_hrz_clip;
		need_clip = true;
	}

	dfc->disp_size = set_bits32(dfc->disp_size, (size_vrt | (size_hrz << 16)), 29, 0);
	dfc->pix_in_num = set_bits32(dfc->pix_in_num, dfc_pix_in_num, 1, 0);
	dfc->disp_fmt = set_bits32(dfc->disp_fmt,
		((dfc_fmt << 1) | (hisi_uv_swap(layer->img.format) << 6) | (hisi_rb_swap(layer->img.format) << 7)), 8, 0);

	if (need_clip) {
		dfc->clip_ctl_hrz = set_bits32(dfc->clip_ctl_hrz,
			(clip_rect.right | (clip_rect.left << 16)), 32, 0);
		dfc->clip_ctl_vrz = set_bits32(dfc->clip_ctl_vrz,
			(clip_rect.bottom | (clip_rect.top << 16)), 32, 0);
		dfc->ctl_clip_en = set_bits32(dfc->ctl_clip_en, 0x1, 1, 0);
	} else {
		dfc->clip_ctl_hrz = set_bits32(dfc->clip_ctl_hrz, 0x0, 32, 0);
		dfc->clip_ctl_vrz = set_bits32(dfc->clip_ctl_vrz, 0x0, 32, 0);
		//FIX_BUG
		dfc->ctl_clip_en = set_bits32(dfc->ctl_clip_en, 0x1, 1, 0);
	}
	dfc->icg_module = set_bits32(dfc->icg_module, 0x1, 1, 0);
	dfc->dither_enable = set_bits32(dfc->dither_enable, 0x0, 1, 0);
	dfc->padding_ctl = set_bits32(dfc->padding_ctl, 0x0, 17, 0);

	//update
	if (need_clip) {
		aligned_rect->w -= (clip_rect.left + clip_rect.right);
		aligned_rect->h -= (clip_rect.top + clip_rect.bottom);
	}

	if (!is_pixel_10bit) {
		dfc->bitext_ctl = set_bits32(dfc->bitext_ctl, 0x3, 32, 0);
	}

	return 0;
}


/*******************************************************************************
** DSS SCF
*/
const int COEF_LUT_TAP4[SCL_COEF_IDX_MAX][PHASE_NUM][TAP4] = {
 	// YUV_COEF_IDX
	{
		{214, 599, 214, -3},
		{207, 597, 223, -3},
		{200, 596, 231, -3},
		{193, 596, 238, -3},
		{186, 595, 246, -3},
		{178, 594, 255, -3},
		{171, 593, 263, -3},
		{165, 591, 271, -3},
		{158, 589, 279, -2},
		{151, 587, 288, -2},
		{145, 584, 296, -1},
		{139, 582, 304, -1},
		{133, 578, 312, 1},
		{127, 575, 321, 1},
		{121, 572, 329, 2},
		{115, 568, 337, 4},
		{109, 564, 346, 5},
		{104, 560, 354, 6},
		{ 98, 555, 362, 9},
		{ 94, 550, 370, 10},
		{ 88, 546, 379, 11},
		{ 84, 540, 387, 13},
		{ 79, 535, 395, 15},
		{ 74, 530, 403, 17},
		{ 70, 524, 411, 19},
		{ 66, 518, 419, 21},
		{ 62, 512, 427, 23},
		{ 57, 506, 435, 26},
		{ 54, 499, 443, 28},
		{ 50, 492, 451, 31},
		{ 47, 486, 457, 34},
		{ 43, 479, 465, 37},
		{ 40, 472, 472, 40},
		{214, 599, 214, -3},
		{207, 597, 223, -3},
		{200, 596, 231, -3},
		{193, 596, 238, -3},
		{186, 595, 246, -3},
		{178, 594, 255, -3},
		{171, 593, 263, -3},
		{165, 591, 271, -3},
		{158, 589, 279, -2},
		{151, 587, 288, -2},
		{145, 584, 296, -1},
		{139, 582, 304, -1},
		{133, 578, 312, 1},
		{127, 575, 321, 1},
		{121, 572, 329, 2},
		{115, 568, 337, 4},
		{109, 564, 346, 5},
		{104, 560, 354, 6},
		{ 98, 555, 362, 9},
		{ 94, 550, 370, 10},
		{ 88, 546, 379, 11},
		{ 84, 540, 387, 13},
		{ 79, 535, 395, 15},
		{ 74, 530, 403, 17},
		{ 70, 524, 411, 19},
		{ 66, 518, 419, 21},
		{ 62, 512, 427, 23},
		{ 57, 506, 435, 26},
		{ 54, 499, 443, 28},
		{ 50, 492, 451, 31},
		{ 47, 486, 457, 34},
		{ 43, 479, 465, 37},
		{ 40, 472, 472, 40}
 	},

	// RGB_COEF_IDX
	{
		{ 0, 1024,   0, 0},
		{ 0, 1008,  16, 0},
		{ 0,  992,  32, 0},
		{ 0,  976,  48, 0},
		{ 0,  960,  64, 0},
		{ 0,  944,  80, 0},
		{ 0,  928,  96, 0},
		{ 0,  912, 112, 0},
		{ 0,  896, 128, 0},
		{ 0,  880, 144, 0},
		{ 0,  864, 160, 0},
		{ 0,  848, 176, 0},
		{ 0,  832, 192, 0},
		{ 0,  816, 208, 0},
		{ 0,  800, 224, 0},
		{ 0,  784, 240, 0},
		{ 0,  768, 256, 0},
		{ 0,  752, 272, 0},
		{ 0,  736, 288, 0},
		{ 0,  720, 304, 0},
		{ 0,  704, 320, 0},
		{ 0,  688, 336, 0},
		{ 0,  672, 352, 0},
		{ 0,  656, 368, 0},
		{ 0,  640, 384, 0},
		{ 0,  624, 400, 0},
		{ 0,  608, 416, 0},
		{ 0,  592, 432, 0},
		{ 0,  576, 448, 0},
		{ 0,  560, 464, 0},
		{ 0,  544, 480, 0},
		{ 0,  528, 496, 0},
		{ 0,  512, 512, 0},
		{ 0, 1024,   0, 0},
		{ 0, 1008,  16, 0},
		{ 0,  992,  32, 0},
		{ 0,  976,  48, 0},
		{ 0,  960,  64, 0},
		{ 0,  944,  80, 0},
		{ 0,  928,  96, 0},
		{ 0,  912, 112, 0},
		{ 0,  896, 128, 0},
		{ 0,  880, 144, 0},
		{ 0,  864, 160, 0},
		{ 0,  848, 176, 0},
		{ 0,  832, 192, 0},
		{ 0,  816, 208, 0},
		{ 0,  800, 224, 0},
		{ 0,  784, 240, 0},
		{ 0,  768, 256, 0},
		{ 0,  752, 272, 0},
		{ 0,  736, 288, 0},
		{ 0,  720, 304, 0},
		{ 0,  704, 320, 0},
		{ 0,  688, 336, 0},
		{ 0,  672, 352, 0},
		{ 0,  656, 368, 0},
		{ 0,  640, 384, 0},
		{ 0,  624, 400, 0},
		{ 0,  608, 416, 0},
		{ 0,  592, 432, 0},
		{ 0,  576, 448, 0},
		{ 0,  560, 464, 0},
		{ 0,  544, 480, 0},
		{ 0,  528, 496, 0},
		{ 0,  512, 512, 0}
 	}
};

const int COEF_LUT_TAP5[SCL_COEF_IDX_MAX][PHASE_NUM][TAP5] = {
	// YUV_COEF_IDX
	{
		{  98, 415, 415,  98, -2},
		{  95, 412, 418, 103, -4},
		{  91, 408, 422, 107, -4},
		{  87, 404, 426, 111, -4},
		{  84, 399, 430, 115, -4},
		{  80, 395, 434, 119, -4},
		{  76, 390, 438, 124, -4},
		{  73, 386, 440, 128, -3},
		{  70, 381, 444, 132, -3},
		{  66, 376, 448, 137, -3},
		{  63, 371, 451, 142, -3},
		{  60, 366, 455, 146, -3},
		{  57, 361, 457, 151, -2},
		{  54, 356, 460, 156, -2},
		{  51, 351, 463, 161, -2},
		{  49, 346, 465, 165, -1},
		{  46, 341, 468, 170, -1},
		{  43, 336, 470, 175, 0},
		{  41, 331, 472, 180, 0},
		{  38, 325, 474, 186, 1},
		{  36, 320, 476, 191, 1},
		{  34, 315, 477, 196, 2},
		{  32, 309, 479, 201, 3},
		{  29, 304, 481, 206, 4},
		{  27, 299, 481, 212, 5},
		{  26, 293, 482, 217, 6},
		{  24, 288, 484, 222, 6},
		{  22, 282, 484, 228, 8},
		{  20, 277, 485, 233, 9},
		{  19, 271, 485, 238, 11},
		{  17, 266, 485, 244, 12},
		{  16, 260, 485, 250, 13},
		{  14, 255, 486, 255, 14},
		{ -94, 608, 608, -94, -4},
		{ -94, 594, 619, -91, -4},
		{ -96, 579, 635, -89, -5},
		{ -96, 563, 650, -87, -6},
		{ -97, 548, 665, -85, -7},
		{ -97, 532, 678, -82, -7},
		{ -98, 516, 693, -79, -8},
		{ -97, 500, 705, -75, -9},
		{ -97, 484, 720, -72, -11},
		{ -97, 468, 733, -68, -12},
		{ -96, 452, 744, -63, -13},
		{ -95, 436, 755, -58, -14},
		{ -94, 419, 768, -53, -16},
		{ -93, 403, 779, -48, -17},
		{ -92, 387, 789, -42, -18},
		{ -90, 371, 799, -36, -20},
		{ -89, 355, 809, -29, -22},
		{ -87, 339, 817, -22, -23},
		{ -86, 324, 826, -15, -25},
		{ -84, 308, 835,  -8, -27},
		{ -82, 293, 842,   0, -29},
		{ -80, 277, 849,   9, -31},
		{ -78, 262, 855,  18, -33},
		{ -75, 247, 860,  27, -35},
		{ -73, 233, 865,  36, -37},
		{ -71, 218, 870,  46, -39},
		{ -69, 204, 874,  56, -41},
		{ -66, 190, 876,  67, -43},
		{ -64, 176, 879,  78, -45},
		{ -62, 163, 882,  89, -48},
		{ -59, 150, 883, 100, -50},
		{ -57, 137, 883, 112, -51},
		{ -55, 125, 884, 125, -55}
	 },

	// RGB_COEF_IDX
	 {
		{0, 512,  512, 0, 0},
		{0, 496,  528, 0, 0},
		{0, 480,  544, 0, 0},
		{0, 464,  560, 0, 0},
		{0, 448,  576, 0, 0},
		{0, 432,  592, 0, 0},
		{0, 416,  608, 0, 0},
		{0, 400,  624, 0, 0},
		{0, 384,  640, 0, 0},
		{0, 368,  656, 0, 0},
		{0, 352,  672, 0, 0},
		{0, 336,  688, 0, 0},
		{0, 320,  704, 0, 0},
		{0, 304,  720, 0, 0},
		{0, 288,  736, 0, 0},
		{0, 272,  752, 0, 0},
		{0, 256,  768, 0, 0},
		{0, 240,  784, 0, 0},
		{0, 224,  800, 0, 0},
		{0, 208,  816, 0, 0},
		{0, 192,  832, 0, 0},
		{0, 176,  848, 0, 0},
		{0, 160,  864, 0, 0},
		{0, 144,  880, 0, 0},
		{0, 128,  896, 0, 0},
		{0, 112,  912, 0, 0},
		{0,  96,  928, 0, 0},
		{0,  80,  944, 0, 0},
		{0,  64,  960, 0, 0},
		{0,  48,  976, 0, 0},
		{0,  32,  992, 0, 0},
		{0,  16, 1008, 0, 0},
		{0,   0, 1024, 0, 0},
		{0, 512,  512, 0, 0},
		{0, 496,  528, 0, 0},
		{0, 480,  544, 0, 0},
		{0, 464,  560, 0, 0},
		{0, 448,  576, 0, 0},
		{0, 432,  592, 0, 0},
		{0, 416,  608, 0, 0},
		{0, 400,  624, 0, 0},
		{0, 384,  640, 0, 0},
		{0, 368,  656, 0, 0},
		{0, 352,  672, 0, 0},
		{0, 336,  688, 0, 0},
		{0, 320,  704, 0, 0},
		{0, 304,  720, 0, 0},
		{0, 288,  736, 0, 0},
		{0, 272,  752, 0, 0},
		{0, 256,  768, 0, 0},
		{0, 240,  784, 0, 0},
		{0, 224,  800, 0, 0},
		{0, 208,  816, 0, 0},
		{0, 192,  832, 0, 0},
		{0, 176,  848, 0, 0},
		{0, 160,  864, 0, 0},
		{0, 144,  880, 0, 0},
		{0, 128,  896, 0, 0},
		{0, 112,  912, 0, 0},
		{0,  96,  928, 0, 0},
		{0,  80,  944, 0, 0},
		{0,  64,  960, 0, 0},
		{0,  48,  976, 0, 0},
		{0,  32,  992, 0, 0},
		{0,  16, 1008, 0, 0},
		{0,   0, 1024, 0, 0}
	 }
 };

const int COEF_LUT_TAP6[SCL_COEF_IDX_MAX][PHASE_NUM][TAP6] = {
	// YUV_COEF_IDX
	{
		{ 2, 264, 500, 264, 2, -8},
		{ 2, 257, 499, 268, 6, -8},
		{ 1, 252, 498, 274, 8, -9},
		{  -1, 246,  498, 281,   9,  -9},
		{  -2, 241,  497, 286,  12, -10},
		{  -3, 235,  497, 292,  13, -10},
		{  -5, 230,  496, 298,  15, -10},
		{  -6, 225,  495, 303,  18, -11},
		{  -7, 219,  494, 309,  20, -11},
		{  -7, 213,  493, 314,  23, -12},
		{  -9, 208,  491, 320,  26, -12},
		{ -10, 203,  490, 325,  28, -12},
		{ -10, 197,  488, 331,  31, -13},
		{ -10, 192,  486, 336,  33, -13},
		{ -12, 186,  485, 342,  36, -13},
		{ -12, 181,  482, 347,  39, -13},
		{ -13, 176,  480, 352,  42, -13},
		{ -14, 171,  478, 358,  45, -14},
		{ -14, 166,  476, 363,  48, -15},
		{ -14, 160,  473, 368,  52, -15},
		{ -14, 155,  470, 373,  55, -15},
		{ -15, 150,  467, 378,  59, -15},
		{ -15, 145,  464, 383,  62, -15},
		{ -16, 141,  461, 388,  65, -15},
		{ -16, 136,  458, 393,  68, -15},
		{ -16, 131,  455, 398,  72, -16},
		{ -16, 126,  451, 402,  77, -16},
		{ -16, 122,  448, 407,  79, -16},
		{ -16, 117,  444, 411,  84, -16},
		{ -17, 113,  441, 416,  87, -16},
		{ -17, 108,  437, 420,  92, -16},
		{ -17, 104,  433, 424,  96, -16},
		{ -17, 100,  429, 429, 100, -17},
		{-187, 105, 1186, 105, -187, 2},
		{-182,  86, 1186, 124, -192, 2},
		{-176,  67, 1185, 143, -197, 2},
		{-170,  49, 1182, 163, -202, 2},
		{-166,  32, 1180, 184, -207, 1},
		{-160,  15, 1176, 204, -212, 1},
		{-155,  -2, 1171, 225, -216, 1},
		{-149, -18, 1166, 246, -221, 0},
		{-145, -34, 1160, 268, -225, 0},
		{-139, -49, 1153, 290, -230,  -1},
		{-134, -63, 1145, 312, -234,  -2},
		{-129, -78, 1137, 334, -238,  -2},
		{-124, -91, 1128, 357, -241,  -5},
		{-119, -104, 1118, 379, -245,  -5},
		{-114, -117, 1107, 402, -248,  -6},
		{-109, -129, 1096, 425, -251,  -8},
		{-104, -141, 1083, 448, -254,  -8},
		{-100, -152, 1071, 471, -257,  -9},
		{ -95, -162, 1057, 494, -259, -11},
		{ -90, -172, 1043, 517, -261, -13},
		{ -86, -181, 1028, 540, -263, -14},
		{ -82, -190, 1013, 563, -264, -16},
		{ -77, -199,  997, 586, -265, -18},
		{ -73, -207,  980, 609, -266, -19},
		{ -69, -214,  963, 632, -266, -22},
		{ -65, -221,  945, 655, -266, -24},
		{ -62, -227,  927, 678, -266, -26},
		{ -58, -233,  908, 700, -265, -28},
		{ -54, -238,  889, 722, -264, -31},
		{ -51, -243,  870, 744, -262, -34},
		{ -48, -247,  850, 766, -260, -37},
		{ -45, -251,  829, 787, -257, -39},
		{ -42, -255,  809, 809, -255, -42}
	},

	// RGB_COEF_IDX
	{
		{ 0, 0, 1024,   0, 0, 0},
		{ 0, 0, 1008,  16, 0, 0},
		{ 0, 0,  992,  32, 0, 0},
		{ 0, 0,  976,  48, 0, 0},
		{ 0, 0,  960,  64, 0, 0},
		{ 0, 0,  944,  80, 0, 0},
		{ 0, 0,  928,  96, 0, 0},
		{ 0, 0,  912, 112, 0, 0},
		{ 0, 0,  896, 128, 0, 0},
		{ 0, 0,  880, 144, 0, 0},
		{ 0, 0,  864, 160, 0, 0},
		{ 0, 0,  848, 176, 0, 0},
		{ 0, 0,  832, 192, 0, 0},
		{ 0, 0,  816, 208, 0, 0},
		{ 0, 0,  800, 224, 0, 0},
		{ 0, 0,  784, 240, 0, 0},
		{ 0, 0,  768, 256, 0, 0},
		{ 0, 0,  752, 272, 0, 0},
		{ 0, 0,  736, 288, 0, 0},
		{ 0, 0,  720, 304, 0, 0},
		{ 0, 0,  704, 320, 0, 0},
		{ 0, 0,  688, 336, 0, 0},
		{ 0, 0,  672, 352, 0, 0},
		{ 0, 0,  656, 368, 0, 0},
		{ 0, 0,  640, 384, 0, 0},
		{ 0, 0,  624, 400, 0, 0},
		{ 0, 0,  608, 416, 0, 0},
		{ 0, 0,  592, 432, 0, 0},
		{ 0, 0,  576, 448, 0, 0},
		{ 0, 0,  560, 464, 0, 0},
		{ 0, 0,  544, 480, 0, 0},
		{ 0, 0,  528, 496, 0, 0},
		{ 0, 0,  512, 512, 0, 0},
		{ 0, 0, 1024,   0, 0, 0},
		{ 0, 0, 1008,  16, 0, 0},
		{ 0, 0,  992,  32, 0, 0},
		{ 0, 0,  976,  48, 0, 0},
		{ 0, 0,  960,  64, 0, 0},
		{ 0, 0,  944,  80, 0, 0},
		{ 0, 0,  928,  96, 0, 0},
		{ 0, 0,  912, 112, 0, 0},
		{ 0, 0,  896, 128, 0, 0},
		{ 0, 0,  880, 144, 0, 0},
		{ 0, 0,  864, 160, 0, 0},
		{ 0, 0,  848, 176, 0, 0},
		{ 0, 0,  832, 192, 0, 0},
		{ 0, 0,  816, 208, 0, 0},
		{ 0, 0,  800, 224, 0, 0},
		{ 0, 0,  784, 240, 0, 0},
		{ 0, 0,  768, 256, 0, 0},
		{ 0, 0,  752, 272, 0, 0},
		{ 0, 0,  736, 288, 0, 0},
		{ 0, 0,  720, 304, 0, 0},
		{ 0, 0,  704, 320, 0, 0},
		{ 0, 0,  688, 336, 0, 0},
		{ 0, 0,  672, 352, 0, 0},
		{ 0, 0,  656, 368, 0, 0},
		{ 0, 0,  640, 384, 0, 0},
		{ 0, 0,  624, 400, 0, 0},
		{ 0, 0,  608, 416, 0, 0},
		{ 0, 0,  592, 432, 0, 0},
		{ 0, 0,  576, 448, 0, 0},
		{ 0, 0,  560, 464, 0, 0},
		{ 0, 0,  544, 480, 0, 0},
		{ 0, 0,  528, 496, 0, 0},
		{ 0, 0,  512, 512, 0, 0}
	}
};

void hisi_dss_scl_init(char __iomem *scl_base, dss_scl_t *s_scl)
{
	if (NULL == scl_base) {
		HISI_FB_ERR("scl_base is NULL");
		return;
	}
	if (NULL == s_scl) {
		HISI_FB_ERR("s_scl is NULL");
		return;
	}

	memset(s_scl, 0, sizeof(dss_scl_t));

	s_scl->en_hscl_str = inp32(scl_base + SCF_EN_HSCL_STR);
	s_scl->en_vscl_str = inp32(scl_base + SCF_EN_VSCL_STR);
	s_scl->h_v_order = inp32(scl_base + SCF_H_V_ORDER);
	s_scl->input_width_height = inp32(scl_base + SCF_INPUT_WIDTH_HEIGHT);
	s_scl->output_width_height = inp32(scl_base + SCF_OUTPUT_WIDTH_HEIGHT);
	s_scl->en_hscl = inp32(scl_base + SCF_EN_HSCL);
	s_scl->en_vscl = inp32(scl_base + SCF_EN_VSCL);
	s_scl->acc_hscl = inp32(scl_base + SCF_ACC_HSCL);
	s_scl->inc_hscl = inp32(scl_base + SCF_INC_HSCL);
	s_scl->inc_vscl = inp32(scl_base + SCF_INC_VSCL);
	s_scl->en_mmp = inp32(scl_base + SCF_EN_MMP);
}

void hisi_dss_scl_set_reg(struct hisi_fb_data_type *hisifd,
	char __iomem *scl_base, dss_scl_t *s_scl)
{
	if (scl_base == NULL) {
		HISI_FB_DEBUG("scl_base is NULL!\n");
		return;
	}

	if (s_scl == NULL) {
		HISI_FB_DEBUG("s_scl is NULL!\n");
		return;
	}

	if (hisifd) {
		hisifd->set_reg(hisifd, scl_base + SCF_EN_HSCL_STR, s_scl->en_hscl_str, 32, 0);
		hisifd->set_reg(hisifd, scl_base + SCF_EN_VSCL_STR, s_scl->en_vscl_str, 32, 0);
		hisifd->set_reg(hisifd, scl_base + SCF_H_V_ORDER, s_scl->h_v_order, 32, 0);
		hisifd->set_reg(hisifd, scl_base + SCF_INPUT_WIDTH_HEIGHT, s_scl->input_width_height, 32, 0);
		hisifd->set_reg(hisifd, scl_base + SCF_OUTPUT_WIDTH_HEIGHT, s_scl->output_width_height, 32, 0);
		hisifd->set_reg(hisifd, scl_base + SCF_EN_HSCL, s_scl->en_hscl, 32, 0);
		hisifd->set_reg(hisifd, scl_base + SCF_EN_VSCL, s_scl->en_vscl, 32, 0);
		hisifd->set_reg(hisifd, scl_base + SCF_ACC_HSCL, s_scl->acc_hscl, 32, 0);
		hisifd->set_reg(hisifd, scl_base + SCF_INC_HSCL, s_scl->inc_hscl, 32, 0);
		hisifd->set_reg(hisifd, scl_base + SCF_INC_VSCL, s_scl->inc_vscl, 32, 0);
		hisifd->set_reg(hisifd, scl_base + SCF_EN_MMP, s_scl->en_mmp, 32, 0);
	} else {
		set_reg(scl_base + SCF_EN_HSCL_STR, s_scl->en_hscl_str, 32, 0);
		set_reg(scl_base + SCF_EN_VSCL_STR, s_scl->en_vscl_str, 32, 0);
		set_reg(scl_base + SCF_H_V_ORDER, s_scl->h_v_order, 32, 0);
		set_reg(scl_base + SCF_INPUT_WIDTH_HEIGHT, s_scl->input_width_height, 32, 0);
		set_reg(scl_base + SCF_OUTPUT_WIDTH_HEIGHT, s_scl->output_width_height, 32, 0);
		set_reg(scl_base + SCF_EN_HSCL, s_scl->en_hscl, 32, 0);
		set_reg(scl_base + SCF_EN_VSCL, s_scl->en_vscl, 32, 0);
		set_reg(scl_base + SCF_ACC_HSCL, s_scl->acc_hscl, 32, 0);
		set_reg(scl_base + SCF_INC_HSCL, s_scl->inc_hscl, 32, 0);
		set_reg(scl_base + SCF_INC_VSCL, s_scl->inc_vscl, 32, 0);
		set_reg(scl_base + SCF_EN_MMP, s_scl->en_mmp, 32, 0);
	}
}

int hisi_dss_scl_write_coefs(struct hisi_fb_data_type *hisifd, bool enable_cmdlist,
	char __iomem *addr, const int **p, int row, int col)
{
	int groups[3] = {0};
	int offset = 0;
	int valid_num = 0;
	int i= 0;
	int j = 0;
	int k = 0;

	if (NULL == hisifd) {
		HISI_FB_ERR("hisifd is NULL");
		return -EINVAL;
	}
	if (NULL == addr) {
		HISI_FB_ERR("addr is NULL");
		return -EINVAL;
	}

	if ((row != PHASE_NUM) || (col < TAP4 || col > TAP6)) {
		HISI_FB_ERR("SCF filter coefficients is err, phase_num = %d, tap_num = %d\n", row, col);
		return -EINVAL;
	}

	/*byte*/
	offset = (col == TAP4) ? 8 : 16;
	valid_num = (offset == 16) ? 3 : 2;

	for (i = 0; i < row; i++) {
		for (j = 0; j < col; j += 2) {
			if ((col % 2) && (j == col -1)) {
				groups[j / 2] = (*((int*)p + i * col + j) & 0xFFF) | (0 << 16);
			} else {
				groups[j / 2] = (*((int*)p + i * col + j) & 0xFFF) | (*((int*)p + i * col + j + 1) << 16);
			}
		}

		for (k = 0; k < valid_num; k++) {
			if (enable_cmdlist) {
				hisifd->set_reg(hisifd, addr + offset * i + k * sizeof(int), groups[k], 32, 0);
			} else {
				set_reg(addr + offset * i + k * sizeof(int), groups[k], 32, 0);
			}
			groups[k] = 0;
		}
	}

	return 0;
}

/*lint -save -e438 -e527*/
int hisi_dss_chn_scl_load_filter_coef_set_reg(struct hisi_fb_data_type *hisifd, bool enable_cmdlist,
	int chn_idx, uint32_t format)
{
	uint32_t module_base = 0;
	char __iomem *h0_y_addr = NULL;
	char __iomem *y_addr = NULL;
	char __iomem *uv_addr = NULL;
	int ret = 0;
	int chn_coef_idx = SCL_COEF_YUV_IDX;

	if (NULL == hisifd) {
		HISI_FB_ERR("hisifd is NULL");
		return -EINVAL;
	}

	if (chn_idx != DSS_RCHN_V2)
		return 0;

	if (isYUV(format)) {
		chn_coef_idx = SCL_COEF_YUV_IDX;
	} else {
		chn_coef_idx = SCL_COEF_RGB_IDX;
	}

	if (g_scf_lut_chn_coef_idx[chn_idx] == chn_coef_idx)
		return 0;

	g_scf_lut_chn_coef_idx[chn_idx] = chn_coef_idx;

	module_base = g_dss_module_base[chn_idx][MODULE_SCL_LUT];
	if (module_base == 0) {
		HISI_FB_ERR("module_base is NULL");
		return -EINVAL;
	}

	h0_y_addr = hisifd->dss_base + module_base + DSS_SCF_H0_Y_COEF_OFFSET;
	y_addr = hisifd->dss_base + module_base +DSS_SCF_Y_COEF_OFFSET;
	uv_addr = hisifd->dss_base + module_base +DSS_SCF_UV_COEF_OFFSET;

	ret = hisi_dss_scl_write_coefs(hisifd, enable_cmdlist, h0_y_addr, (const int **)COEF_LUT_TAP6[chn_coef_idx], PHASE_NUM, TAP6);
	if (ret < 0) {
		HISI_FB_ERR("Error to write H0_Y_COEF coefficients.\n");
	}

	ret = hisi_dss_scl_write_coefs(hisifd, enable_cmdlist, y_addr, (const int **)COEF_LUT_TAP5[chn_coef_idx], PHASE_NUM, TAP5);
	if (ret < 0) {
		HISI_FB_ERR("Error to write Y_COEF coefficients.\n");
	}

	ret = hisi_dss_scl_write_coefs(hisifd, enable_cmdlist, uv_addr, (const int **)COEF_LUT_TAP4[chn_coef_idx], PHASE_NUM, TAP4);
	if (ret < 0) {
		HISI_FB_ERR("Error to write UV_COEF coefficients.\n");
	}

	return ret;
}
/*lint -restore*/


int hisi_dss_scl_coef_on(struct hisi_fb_data_type *hisifd, bool enable_cmdlist, int coef_lut_idx)
{
	int i = 0;
	uint32_t module_base = 0;
	char __iomem *h0_y_addr = NULL;
	char __iomem *y_addr = NULL;
	char __iomem *uv_addr = NULL;
	int ret = 0;

	if (NULL == hisifd) {
		HISI_FB_ERR("hisifd is NULL");
		return -EINVAL;
	}

	for (i = 0; i < DSS_CHN_MAX_DEFINE; i++) {
		module_base = g_dss_module_base[i][MODULE_SCL_LUT];
		if (module_base != 0) {
			h0_y_addr = hisifd->dss_base + module_base + DSS_SCF_H0_Y_COEF_OFFSET;
			y_addr = hisifd->dss_base + module_base +DSS_SCF_Y_COEF_OFFSET;
			uv_addr = hisifd->dss_base + module_base +DSS_SCF_UV_COEF_OFFSET;

			g_scf_lut_chn_coef_idx[i] = coef_lut_idx;

			ret = hisi_dss_scl_write_coefs(hisifd, enable_cmdlist, h0_y_addr, (const int **)COEF_LUT_TAP6[coef_lut_idx], PHASE_NUM, TAP6);
			if (ret < 0) {
				HISI_FB_ERR("Error to write H0_Y_COEF coefficients.\n");
			}

			if (i == DSS_RCHN_V0) {
				hisi_dss_arsr2p_coef_on(hisifd, enable_cmdlist);
				continue;
			}

			ret = hisi_dss_scl_write_coefs(hisifd, enable_cmdlist, y_addr, (const int **)COEF_LUT_TAP5[coef_lut_idx], PHASE_NUM, TAP5);
			if (ret < 0) {
				HISI_FB_ERR("Error to write Y_COEF coefficients.\n");
			}

			ret = hisi_dss_scl_write_coefs(hisifd, enable_cmdlist, uv_addr, (const int **)COEF_LUT_TAP4[coef_lut_idx], PHASE_NUM, TAP4);
			if (ret < 0) {
				HISI_FB_ERR("Error to write UV_COEF coefficients.\n");
			}
		}
	}

	return 0;
}


int hisi_dss_scl_config(struct hisi_fb_data_type *hisifd,
	dss_layer_t *layer, dss_rect_t *aligned_rect, bool rdma_stretch_enable)
{
	dss_scl_t *scl = NULL;
	dss_rect_t src_rect;
	dss_rect_t dst_rect;
	uint32_t need_cap = 0;
	int chn_idx = 0;
	uint32_t transform = 0;
	dss_block_info_t *pblock_info = NULL;

	bool has_pixel_alpha = false;
	bool en_hscl = false;
	bool en_vscl = false;
	bool en_mmp = false;
	uint32_t h_ratio = 0;
	uint32_t v_ratio = 0;
	uint32_t h_v_order = 0;
	uint32_t acc_hscl = 0;
	uint32_t acc_vscl = 0;
	uint32_t scf_en_vscl = 0;
	uint32_t chn_idx_temp;

	if (NULL == hisifd) {
		HISI_FB_ERR("hisifd is NULL");
		return -EINVAL;
	}
	if (NULL == layer) {
		HISI_FB_ERR("layer is NULL");
		return -EINVAL;
	}

	if (hisifd->index == MEDIACOMMON_PANEL_IDX) {
		return 0;
	}

	need_cap = layer->need_cap;
	chn_idx = layer->chn_idx;
	transform = layer->transform;
	chn_idx_temp = DSS_RCHN_V0;

	if (aligned_rect)
		src_rect = *aligned_rect;
	else
		src_rect = layer->src_rect;
	dst_rect = layer->dst_rect;
	pblock_info = &(layer->block_info);

	if (pblock_info && pblock_info->both_vscfh_arsr2p_used) {
		dst_rect = pblock_info->arsr2p_in_rect;
	}

	if (chn_idx == chn_idx_temp) {
		dst_rect.h = src_rect.h;//set dst height to src height, disable vertical scaling in v0scf
	}

	do {
		if (chn_idx == chn_idx_temp && pblock_info->h_ratio_arsr2p && pblock_info->h_ratio) {
			h_ratio = pblock_info->h_ratio;
			en_hscl = true; //v0 channel,  both scf and arsr2p are enabled
			break;
		} else if (chn_idx == chn_idx_temp && !pblock_info->h_ratio && pblock_info->h_ratio_arsr2p) {
			break;   //v0 channel , vscf is not supported, just break;
		}

		if (pblock_info && (pblock_info->h_ratio != 0) && (pblock_info->h_ratio != SCF_INC_FACTOR)) {
			h_ratio = pblock_info->h_ratio;
			en_hscl = true;
			break;
		}

		if (chn_idx == chn_idx_temp) {
			dst_rect.w = (src_rect.w > dst_rect.w ? dst_rect.w : src_rect.w);  //disable horizental scaling up
		}

		if (src_rect.w == dst_rect.w)
			break;

		en_hscl = true;

		if ((src_rect.w < SCF_MIN_INPUT) || (dst_rect.w < SCF_MIN_OUTPUT)) {
			HISI_FB_ERR("src_rect.w(%d) small than 16, or dst_rect.w(%d) small than 16\n",
				src_rect.w, dst_rect.w);
			return -EINVAL;
		}

		//h_ratio = DSS_WIDTH(src_rect.w) * SCF_INC_FACTOR / DSS_WIDTH(dst_rect.w);
		h_ratio = (DSS_HEIGHT(src_rect.w) * SCF_INC_FACTOR + SCF_INC_FACTOR / 2 - acc_hscl) /
			DSS_HEIGHT(dst_rect.w);

		if ((dst_rect.w > (src_rect.w * SCF_UPSCALE_MAX))
			|| (src_rect.w > (dst_rect.w * SCF_DOWNSCALE_MAX))) {
			HISI_FB_ERR("width out of range, original_src_rec(%d, %d, %d, %d) "
				"new_src_rect(%d, %d, %d, %d), dst_rect(%d, %d, %d, %d), rdma_stretch_enable=%d\n",
				layer->src_rect.x, layer->src_rect.y, layer->src_rect.w, layer->src_rect.h,
				src_rect.x, src_rect.y, src_rect.w, src_rect.h,
				dst_rect.x, dst_rect.y, dst_rect.w, dst_rect.h, rdma_stretch_enable);

			return -EINVAL;
		}
	} while(0);

	do {
		if (src_rect.h == dst_rect.h)
			break;

		en_vscl = true;
		scf_en_vscl = 1;

		v_ratio = (DSS_HEIGHT(src_rect.h) * SCF_INC_FACTOR + SCF_INC_FACTOR / 2 - acc_vscl) /
			DSS_HEIGHT(dst_rect.h);

		if ((dst_rect.h > (src_rect.h * SCF_UPSCALE_MAX))
			|| (src_rect.h > (dst_rect.h * SCF_DOWNSCALE_MAX))) {
			HISI_FB_ERR("height out of range, original_src_rec(%d, %d, %d, %d) "
				"new_src_rect(%d, %d, %d, %d), dst_rect(%d, %d, %d, %d), rdma_stretch_enable=%d.\n",
				layer->src_rect.x, layer->src_rect.y, layer->src_rect.w, layer->src_rect.h,
				src_rect.x, src_rect.y, src_rect.w, src_rect.h,
				dst_rect.x, dst_rect.y, dst_rect.w, dst_rect.h, rdma_stretch_enable);
			return -EINVAL;
		}
	} while(0);

	if (!en_hscl && !en_vscl) {

		return 0;
	}

	/* scale down, do hscl first; scale up, do vscl first*/
	h_v_order = (src_rect.w > dst_rect.w) ? 0 : 1;

	if (pblock_info && (pblock_info->acc_hscl != 0)) {
		acc_hscl = pblock_info->acc_hscl;
	}

	scl = &(hisifd->dss_module.scl[chn_idx]);
	hisifd->dss_module.scl_used[chn_idx] = 1;

	has_pixel_alpha = hal_format_has_alpha(layer->img.format);

	scl->en_hscl_str = set_bits32(scl->en_hscl_str, 0x0, 1, 0);

	//if (DSS_HEIGHT(src_rect.h) * 2 >= DSS_HEIGHT(dst_rect.h)) {
	if (v_ratio >= 2 * SCF_INC_FACTOR) {
		if (has_pixel_alpha)
			scl->en_vscl_str = set_bits32(scl->en_vscl_str, 0x3, 2, 0);
		else
			scl->en_vscl_str = set_bits32(scl->en_vscl_str, 0x1, 2, 0);
	} else {
		scl->en_vscl_str = set_bits32(scl->en_vscl_str, 0x0, 1, 0);
	}

	if (src_rect.h > dst_rect.h) {
		scf_en_vscl = 0x3;
	}
	en_mmp = 0x1;

	scl->h_v_order = set_bits32(scl->h_v_order, h_v_order, 1, 0);
	scl->input_width_height = set_bits32(scl->input_width_height,
		DSS_HEIGHT(src_rect.h), 13, 0);
	scl->input_width_height = set_bits32(scl->input_width_height,
		DSS_WIDTH(src_rect.w), 13, 16);
	scl->output_width_height = set_bits32(scl->output_width_height,
		DSS_HEIGHT(dst_rect.h), 13, 0);
	scl->output_width_height = set_bits32(scl->output_width_height,
		DSS_WIDTH(dst_rect.w), 13, 16);
	scl->en_hscl = set_bits32(scl->en_hscl, (en_hscl ? 0x1 : 0x0), 1, 0);
	scl->en_vscl = set_bits32(scl->en_vscl, scf_en_vscl, 2, 0);
	scl->acc_hscl = set_bits32(scl->acc_hscl, acc_hscl, 31, 0);
	scl->inc_hscl = set_bits32(scl->inc_hscl, h_ratio, 24, 0);
	scl->inc_vscl = set_bits32(scl->inc_vscl, v_ratio, 24, 0);
	scl->en_mmp = set_bits32(scl->en_mmp, en_mmp, 1, 0);
	scl->fmt = layer->img.format;

	return 0;
}

uint32_t get_rdma_stretch_line_num(dss_layer_t *layer)
{
	uint32_t v_stretch_ratio_threshold = 0;
	uint32_t v_stretch_line_num = 0;

	if ((layer->need_cap & CAP_AFBCD) || (layer->need_cap & CAP_HFBCD)) {
		v_stretch_line_num = 0;
	} else {
		if (is_YUV_SP_420(layer->img.format) || is_YUV_P_420(layer->img.format)) {
			v_stretch_ratio_threshold = ((layer->src_rect.h + layer->dst_rect.h - 1) / layer->dst_rect.h);
			v_stretch_line_num = ((layer->src_rect.h / layer->dst_rect.h) / 2) * 2;
		} else {
			v_stretch_ratio_threshold = ((layer->src_rect.h + layer->dst_rect.h - 1) / layer->dst_rect.h);
			v_stretch_line_num = (layer->src_rect.h / layer->dst_rect.h);
		}

		if (v_stretch_ratio_threshold <= g_rdma_stretch_threshold) {//lint !e574 !e737
			v_stretch_line_num = 0;
		}
	}

	return v_stretch_line_num;
}



/*******************************************************************************
** DSS POST_CLIP
*/
void hisi_dss_post_clip_init(char __iomem *post_clip_base,
	dss_post_clip_t *s_post_clip)
{
	if (NULL == post_clip_base) {
		HISI_FB_ERR("post_clip_base is NULL");
		return;
	}
	if (NULL == s_post_clip) {
		HISI_FB_ERR("s_post_clip is NULL");
		return;
	}

	memset(s_post_clip, 0, sizeof(dss_post_clip_t));
}

int hisi_dss_post_clip_config(struct hisi_fb_data_type *hisifd, dss_layer_t *layer)
{
	dss_post_clip_t *post_clip = NULL;
	int chn_idx = 0;
	dss_rect_t post_clip_rect;

	chn_idx = layer->chn_idx;
	post_clip_rect = layer->dst_rect;

	if (hisifd->index == MEDIACOMMON_PANEL_IDX) {
		return 0;
	}

	if (((chn_idx >= DSS_RCHN_V0) && (chn_idx <= DSS_RCHN_G1)) || (chn_idx == DSS_RCHN_V2))
	{
		post_clip = &(hisifd->dss_module.post_clip[chn_idx]);
		hisifd->dss_module.post_clip_used[chn_idx] = 1;

		post_clip->disp_size = set_bits32(post_clip->disp_size, DSS_HEIGHT(post_clip_rect.h), 13, 0);
		post_clip->disp_size = set_bits32(post_clip->disp_size, DSS_WIDTH(post_clip_rect.w), 13, 16);
		if ((chn_idx == DSS_RCHN_V0) && layer->block_info.arsr2p_left_clip) {
			post_clip->clip_ctl_hrz = set_bits32(post_clip->clip_ctl_hrz, layer->block_info.arsr2p_left_clip, 6, 16);
			post_clip->clip_ctl_hrz = set_bits32(post_clip->clip_ctl_hrz, 0x0, 6, 0);
		} else {
			post_clip->clip_ctl_hrz = set_bits32(post_clip->clip_ctl_hrz, 0x0, 32, 0);
		}

		post_clip->clip_ctl_vrz = set_bits32(post_clip->clip_ctl_vrz, 0x0, 32, 0);
		post_clip->ctl_clip_en = set_bits32(post_clip->ctl_clip_en, 0x1, 32, 0);
	}

	return 0;
}

/*******************************************************************************
** DSS CE
*/

static int hisi_dss_dpp_update_acm_lut_table(struct hisi_fb_data_type *hisifd,uint32_t **local_acm_lut_table,
	uint32_t *local_r_hh,uint32_t *local_r_lh) {

	struct hisi_panel_info *pinfo = &(hisifd->panel_info);
	acm_reg_t *cur_acm_reg = &(hisifd->acm_reg);

	if (SCENE_MODE_GALLERY == hisifd->user_scene_mode || SCENE_MODE_DEFAULT == hisifd->user_scene_mode) {
		if (pinfo->acm_lut_hue_table_len > 0 && pinfo->acm_lut_hue_table
			&& pinfo->acm_lut_sata_table_len > 0 && pinfo->acm_lut_sata_table
			&& pinfo->acm_lut_satr0_table_len > 0 && pinfo->acm_lut_satr0_table
			&& pinfo->acm_lut_satr1_table_len > 0 && pinfo->acm_lut_satr1_table
			&& pinfo->acm_lut_satr2_table_len > 0 && pinfo->acm_lut_satr2_table
			&& pinfo->acm_lut_satr3_table_len > 0 && pinfo->acm_lut_satr3_table
			&& pinfo->acm_lut_satr4_table_len > 0 && pinfo->acm_lut_satr4_table
			&& pinfo->acm_lut_satr5_table_len > 0 && pinfo->acm_lut_satr5_table
			&& pinfo->acm_lut_satr6_table_len > 0 && pinfo->acm_lut_satr6_table
			&& pinfo->acm_lut_satr7_table_len > 0 && pinfo->acm_lut_satr7_table) {
			hisi_effect_color_dimming_acm_reg_set(hisifd,cur_acm_reg);
			local_acm_lut_table[0] = cur_acm_reg->acm_lut_hue_table;
			local_acm_lut_table[1] = cur_acm_reg->acm_lut_sata_table;
			local_acm_lut_table[2] = cur_acm_reg->acm_lut_satr0_table;
			local_acm_lut_table[3] = cur_acm_reg->acm_lut_satr1_table;
			local_acm_lut_table[4] = cur_acm_reg->acm_lut_satr2_table;
			local_acm_lut_table[5] = cur_acm_reg->acm_lut_satr3_table;
			local_acm_lut_table[6] = cur_acm_reg->acm_lut_satr4_table;
			local_acm_lut_table[7] = cur_acm_reg->acm_lut_satr5_table;
			local_acm_lut_table[8] = cur_acm_reg->acm_lut_satr6_table;
			local_acm_lut_table[9] = cur_acm_reg->acm_lut_satr7_table;
			local_r_hh[0] = pinfo->r0_hh;
			local_r_lh[0] = pinfo->r0_lh;
			local_r_hh[1]  = pinfo->r1_hh;
			local_r_lh[1] = pinfo->r1_lh;
			local_r_hh[2]  = pinfo->r2_hh;
			local_r_lh[2] = pinfo->r2_lh;
			local_r_hh[3]  = pinfo->r3_hh;
			local_r_lh[3] = pinfo->r3_lh;
			local_r_hh[4]  = pinfo->r4_hh;
			local_r_lh[4] = pinfo->r4_lh;
			local_r_hh[5]  = pinfo->r5_hh;
			local_r_lh[5] = pinfo->r5_lh;
			local_r_hh[6]  = pinfo->r6_hh;
			local_r_lh[6] = pinfo->r6_lh;
			return  TRUE;
		}
	} else if (SCENE_MODE_VIDEO == hisifd->user_scene_mode) {
		if (pinfo->acm_lut_hue_table_len > 0 && pinfo->video_acm_lut_hue_table
			&& pinfo->acm_lut_sata_table_len > 0 && pinfo->video_acm_lut_sata_table
			&& pinfo->acm_lut_satr0_table_len > 0 && pinfo->video_acm_lut_satr0_table
			&& pinfo->acm_lut_satr1_table_len > 0 && pinfo->video_acm_lut_satr1_table
			&& pinfo->acm_lut_satr2_table_len > 0 && pinfo->video_acm_lut_satr2_table
			&& pinfo->acm_lut_satr3_table_len > 0 && pinfo->video_acm_lut_satr3_table
			&& pinfo->acm_lut_satr4_table_len > 0 && pinfo->video_acm_lut_satr4_table
			&& pinfo->acm_lut_satr5_table_len > 0 && pinfo->video_acm_lut_satr5_table
			&& pinfo->acm_lut_satr6_table_len > 0 && pinfo->video_acm_lut_satr6_table
			&& pinfo->acm_lut_satr7_table_len > 0 && pinfo->video_acm_lut_satr7_table) {
			hisi_effect_color_dimming_acm_reg_set(hisifd,cur_acm_reg);
			local_acm_lut_table[0] = cur_acm_reg->acm_lut_hue_table;
			local_acm_lut_table[1] = cur_acm_reg->acm_lut_sata_table;
			local_acm_lut_table[2] = cur_acm_reg->acm_lut_satr0_table;
			local_acm_lut_table[3] = cur_acm_reg->acm_lut_satr1_table;
			local_acm_lut_table[4] = cur_acm_reg->acm_lut_satr2_table;
			local_acm_lut_table[5] = cur_acm_reg->acm_lut_satr3_table;
			local_acm_lut_table[6] = cur_acm_reg->acm_lut_satr4_table;
			local_acm_lut_table[7] = cur_acm_reg->acm_lut_satr5_table;
			local_acm_lut_table[8] = cur_acm_reg->acm_lut_satr6_table;
			local_acm_lut_table[9] = cur_acm_reg->acm_lut_satr7_table;
			local_r_hh[0] = pinfo->video_r0_hh;
			local_r_lh[0]  = pinfo->video_r0_lh;
			local_r_hh[1] = pinfo->video_r1_hh;
			local_r_lh[1]  = pinfo->video_r1_lh;
			local_r_hh[2] = pinfo->video_r2_hh;
			local_r_lh[2]  = pinfo->video_r2_lh;
			local_r_hh[3] = pinfo->video_r3_hh;
			local_r_lh[3]  = pinfo->video_r3_lh;
			local_r_hh[4] = pinfo->video_r4_hh;
			local_r_lh[4]  = pinfo->video_r4_lh;
			local_r_hh[5] = pinfo->video_r5_hh;
			local_r_lh[5]  = pinfo->video_r5_lh;
			local_r_hh[6] = pinfo->video_r6_hh;
			local_r_lh[6]  = pinfo->video_r6_lh;
			return  TRUE;
		}
		return FALSE;
	}}

static void hisi_dss_dpp_acm_gmp_set_reg(struct hisi_fb_data_type *hisifd) {
	struct hisi_panel_info *pinfo = NULL;
	char __iomem *lcp_base = NULL;
	char __iomem *acm_base = NULL;
	char __iomem *acm_lut_base = NULL;
	static int last_user_scene_mode = 0;
	uint32_t lut_sel = 0;
	bool need_update_acm = FALSE;
	acm_reg_t *cur_acm_reg = NULL;

	uint32_t local_r_hh[7] = {0};
	uint32_t local_r_lh[7] = {0};
	uint32_t *local_acm_lut_table[10] = {NULL};

	if (NULL == hisifd) {
		HISI_FB_ERR("hisifd, NUll pointer warning.\n");
		goto func_exit;
	}

	pinfo = &(hisifd->panel_info);

	if ((0 == pinfo->gmp_support && 0 == pinfo->acm_support) || 0 == pinfo->smart_color_mode_support) {
		goto func_exit;
	}

	cur_acm_reg = &(hisifd->acm_reg);

	if (!HISI_DSS_SUPPORT_DPP_MODULE_BIT(DPP_MODULE_LCP_GMP) || !HISI_DSS_SUPPORT_DPP_MODULE_BIT(DPP_MODULE_ACM)) {
		HISI_FB_DEBUG("gmp or acm are not suppportted in this platform.\n");
		goto func_exit;
	}

	if (PRIMARY_PANEL_IDX == hisifd->index) {
		lcp_base = hisifd->dss_base + DSS_DPP_LCP_OFFSET;
		acm_base = hisifd->dss_base + DSS_DPP_ACM_OFFSET;
		acm_lut_base = hisifd->dss_base + DSS_DPP_ACM_LUT_OFFSET;
	} else {
		HISI_FB_ERR("fb%d, not support!\n", hisifd->index);
		goto func_exit;
	}

	if (last_user_scene_mode != hisifd->user_scene_mode) {
		last_user_scene_mode = hisifd->user_scene_mode;
		hisifd->dimming_count = DIMMING_COUNT;
	} else if(hisifd->dimming_count > 0){
		(hisifd->dimming_count)--;
	} else {
		goto func_exit;
	}

	if (pinfo->gmp_support) {
		if (SCENE_MODE_GALLERY == hisifd->user_scene_mode || SCENE_MODE_DEFAULT == hisifd->user_scene_mode) {
			//enable gmp
			set_reg(lcp_base + LCP_GMP_BYPASS_EN, 0x0, 1, 0);
		} else if (SCENE_MODE_VIDEO == hisifd->user_scene_mode) {
			//disable gmp
			set_reg(lcp_base + LCP_GMP_BYPASS_EN, 0x1, 1, 0);
		}
	}

	if (pinfo->acm_support) {
		need_update_acm = hisi_dss_dpp_update_acm_lut_table(hisifd,local_acm_lut_table,local_r_hh,local_r_lh);
		if (need_update_acm) {
			set_reg(acm_base + ACM_R0_H, ((local_r_hh[0] & 0x3ff) << 16) | (local_r_lh[0] & 0x3ff), 32, 0);
			set_reg(acm_base + ACM_R1_H, ((local_r_hh[1] & 0x3ff) << 16) | (local_r_lh[1] & 0x3ff), 32, 0);
			set_reg(acm_base + ACM_R2_H, ((local_r_hh[2] & 0x3ff) << 16) | (local_r_lh[2] & 0x3ff), 32, 0);
			set_reg(acm_base + ACM_R3_H, ((local_r_hh[3] & 0x3ff) << 16) | (local_r_lh[3] & 0x3ff), 32, 0);
			set_reg(acm_base + ACM_R4_H, ((local_r_hh[4] & 0x3ff) << 16) | (local_r_lh[4] & 0x3ff), 32, 0);
			set_reg(acm_base + ACM_R5_H, ((local_r_hh[5] & 0x3ff) << 16) | (local_r_lh[5] & 0x3ff), 32, 0);
			set_reg(acm_base + ACM_R6_H, ((local_r_hh[6] & 0x3ff) << 16) | (local_r_lh[6] & 0x3ff), 32, 0);
			acm_set_lut_hue(acm_lut_base + ACM_U_H_COEF, local_acm_lut_table[0], pinfo->acm_lut_hue_table_len);
			acm_set_lut(acm_lut_base + ACM_U_SATA_COEF, local_acm_lut_table[1], pinfo->acm_lut_sata_table_len);
			acm_set_lut(acm_lut_base + ACM_U_SATR0_COEF, local_acm_lut_table[2], pinfo->acm_lut_satr0_table_len);
			acm_set_lut(acm_lut_base + ACM_U_SATR1_COEF, local_acm_lut_table[3], pinfo->acm_lut_satr1_table_len);
			acm_set_lut(acm_lut_base + ACM_U_SATR2_COEF, local_acm_lut_table[4], pinfo->acm_lut_satr2_table_len);
			acm_set_lut(acm_lut_base + ACM_U_SATR3_COEF, local_acm_lut_table[5], pinfo->acm_lut_satr3_table_len);
			acm_set_lut(acm_lut_base + ACM_U_SATR4_COEF, local_acm_lut_table[6], pinfo->acm_lut_satr4_table_len);
			acm_set_lut(acm_lut_base + ACM_U_SATR5_COEF, local_acm_lut_table[7], pinfo->acm_lut_satr5_table_len);
			acm_set_lut(acm_lut_base + ACM_U_SATR6_COEF, local_acm_lut_table[8], pinfo->acm_lut_satr6_table_len);
			acm_set_lut(acm_lut_base + ACM_U_SATR7_COEF, local_acm_lut_table[9], pinfo->acm_lut_satr7_table_len);

			lut_sel = inp32(acm_base + ACM_LUT_SEL);
			set_reg(acm_base + ACM_LUT_SEL, (~lut_sel) & 0x3FF, 10, 0);
			//enable acm
			set_reg(acm_base + ACM_EN, 0x1, 1, 0);
		}
	}

func_exit:
	return;
}

/*******************************************************************************
** DSS MCTL
*/
void hisi_dss_mctl_init(char __iomem *mctl_base, dss_mctl_t *s_mctl)
{
	if (NULL == mctl_base) {
		HISI_FB_ERR("mctl_base is NULL");
		return;
	}
	if (NULL == s_mctl) {
		HISI_FB_ERR("s_mctl is NULL");
		return;
	}

	memset(s_mctl, 0, sizeof(dss_mctl_t));

	//FIXME:
}

static void hisi_dss_mctl_ch_starty_init(char __iomem *mctl_ch_starty_base, dss_mctl_ch_t *s_mctl_ch)
{
	if (NULL == mctl_ch_starty_base) {
		HISI_FB_ERR("mctl_ch_starty_base is NULL");
		return;
	}
	if (NULL == s_mctl_ch) {
		HISI_FB_ERR("s_mctl_ch is NULL");
		return;
	}

	memset(s_mctl_ch, 0, sizeof(dss_mctl_ch_t));

	s_mctl_ch->chn_starty = inp32(mctl_ch_starty_base);
}

void hisi_dss_mctl_ch_mod_dbg_init(char __iomem *mctl_ch_dbg_base, dss_mctl_ch_t *s_mctl_ch)
{
	if (NULL == mctl_ch_dbg_base) {
		HISI_FB_ERR("mctl_ch_dbg_base is NULL");
		return;
	}
	if (NULL == s_mctl_ch) {
		HISI_FB_ERR("s_mctl_ch is NULL");
		return;
	}

	s_mctl_ch->chn_mod_dbg = inp32(mctl_ch_dbg_base);
}

static void hisi_dss_mctl_sys_set_reg(struct hisi_fb_data_type *hisifd,
	char __iomem *mctl_sys_base, dss_mctl_sys_t *s_mctl_sys, int ovl_idx)
{
	int k = 0;

	if (hisifd == NULL) {
		HISI_FB_DEBUG("hisifd is NULL!\n");
		return;
	}

	if (mctl_sys_base == NULL) {
		HISI_FB_DEBUG("mctl_sys_base is NULL!\n");
		return;
	}

	if (s_mctl_sys == NULL) {
		HISI_FB_DEBUG("s_mctl_sys is NULL!\n");
		return;
	}

	if ((ovl_idx < DSS_OVL0) || (ovl_idx >= DSS_OVL_IDX_MAX)) {
		HISI_FB_DEBUG("ovl_idx is out of the range!\n");
		return;
	}

	if (s_mctl_sys->chn_ov_sel_used[ovl_idx]) {
		hisifd->set_reg(hisifd, mctl_sys_base + MCTL_RCH_OV0_SEL + (ovl_idx * 0x4),
			s_mctl_sys->chn_ov_sel[ovl_idx], 32, 0);

	}

	for (k = 0; k < DSS_WCH_MAX; k++) {
		if (s_mctl_sys->wch_ov_sel_used[k]) {
			hisifd->set_reg(hisifd, mctl_sys_base + MCTL_WCH_OV2_SEL + (k * 0x4),
				s_mctl_sys->wchn_ov_sel[k], 32, 0);
		}
	}

	if (s_mctl_sys->ov_flush_en_used[ovl_idx]) {
		hisifd->set_reg(hisifd, mctl_sys_base + MCTL_OV0_FLUSH_EN + (ovl_idx * 0x4),
			s_mctl_sys->ov_flush_en[ovl_idx], 32, 0);
	}
}

static void hisi_dss_mctl_ov_set_reg(struct hisi_fb_data_type *hisifd,
	char __iomem *mctl_base, dss_mctl_t *s_mctl, int ovl_idx, bool enable_cmdlist)
{
	if (hisifd == NULL) {
		HISI_FB_DEBUG("hisifd is NULL!\n");
		return;
	}

	if (mctl_base == NULL) {
		HISI_FB_DEBUG("mctl_base is NULL!\n");
		return;
	}

	if (s_mctl == NULL) {
		HISI_FB_DEBUG("s_mctl is NULL!\n");
		return;
	}

	if ((ovl_idx < DSS_OVL0) || (ovl_idx >= DSS_OVL_IDX_MAX)) {
		HISI_FB_DEBUG("ovl_idx  is is out of the range !\n");
		return;
	}

	if ((ovl_idx == DSS_OVL0) || (ovl_idx == DSS_OVL1)) {
		hisifd->set_reg(hisifd, mctl_base+ MCTL_CTL_MUTEX_DBUF, s_mctl->ctl_mutex_dbuf, 32, 0);
		hisi_dss_mctl_ov_set_ctl_dbg_reg(hisifd, mctl_base, enable_cmdlist);
	}

	hisifd->set_reg(hisifd, mctl_base + MCTL_CTL_MUTEX_OV, s_mctl->ctl_mutex_ov, 32, 0);
}
/*lint -e715*/
static void hisi_dss_mctl_ch_set_reg(struct hisi_fb_data_type *hisifd,
	dss_mctl_ch_base_t *mctl_ch_base, dss_mctl_ch_t *s_mctl_ch, int32_t mctl_idx, int chn_idx)
{
	char __iomem *chn_mutex_base = NULL;
	int i = 0;

	if (hisifd == NULL) {
		HISI_FB_DEBUG("hisifd is NULL!\n");
		return;
	}

	if (mctl_ch_base == NULL) {
		HISI_FB_DEBUG("mctl_ch_base is NULL!\n");
		return;
	}

	if (s_mctl_ch == NULL) {
		HISI_FB_DEBUG("s_mctl_chis NULL!\n");
		return;
	}

	if ((mctl_idx < DSS_MCTL0) || (mctl_idx >= DSS_MCTL_IDX_MAX)) {
		HISI_FB_DEBUG("mctl_idx is out of the range!\n");
		return;
	}

	if (hisifd->index == MEDIACOMMON_PANEL_IDX) {
		chn_mutex_base = mctl_ch_base->chn_mutex_base;
		hisifd->set_reg(hisifd, chn_mutex_base, s_mctl_ch->chn_mutex, 32, 0);
	} else {
		for (i = 0; i < DSS_MCTL_IDX_MAX; i++) {
			if (g_dss_module_ovl_base[i][MODULE_MCTL_BASE] == 0) continue;
			chn_mutex_base = mctl_ch_base->chn_mutex_base +
	                         g_dss_module_ovl_base[i][MODULE_MCTL_BASE];

			if (i != mctl_idx) {
				hisifd->set_reg(hisifd, chn_mutex_base, 0, 32, 0);
			}
		}

		chn_mutex_base = mctl_ch_base->chn_mutex_base +
			g_dss_module_ovl_base[mctl_idx][MODULE_MCTL_BASE];
		if (NULL == chn_mutex_base) {
			HISI_FB_DEBUG("chn_mutex_base is NULL!\n");
			return;
		}

		hisifd->set_reg(hisifd, chn_mutex_base, s_mctl_ch->chn_mutex, 32, 0);
	}
}

static void hisi_dss_mctl_sys_ch_set_reg(struct hisi_fb_data_type *hisifd,
	dss_mctl_ch_base_t *mctl_ch_base, dss_mctl_ch_t *s_mctl_ch, int chn_idx, bool normal)
{
	char __iomem *mctl_sys_base = NULL;

	if (hisifd == NULL) {
		HISI_FB_DEBUG("hisifd is NULL!\n");
		return;
	}

	if (mctl_ch_base == NULL) {
		HISI_FB_DEBUG("mctl_ch_base is NULL!\n");
		return;
	}

	if (s_mctl_ch == NULL) {
		HISI_FB_DEBUG("s_mctl_ch is NULL!\n");
		return;
	}

	//if (chn_idx < DSS_WCHN_W0) {
		//hisifd->set_reg(hisifd, mctl_ch_base->chn_starty_base, s_mctl_ch->chn_starty, 32, 0);
	//}

	mctl_sys_base = hisifd->dss_base + DSS_MCTRL_SYS_OFFSET;

	if (normal == true) {    //copybit
		if (chn_idx == DSS_RCHN_V2) {
			hisifd->set_reg(hisifd, mctl_sys_base + MCTL_MOD19_DBG, 0xA0000, 32, 0);
		}

		if (chn_idx == DSS_WCHN_W2) {
			hisifd->set_reg(hisifd, mctl_sys_base + MCTL_MOD20_DBG, 0xA0000, 32, 0);
		}
	}

	if (normal == false) {
		if (chn_idx == DSS_RCHN_V2) {
			hisifd->set_reg(hisifd, mctl_sys_base + MCTL_MOD19_DBG, 0xA0002, 32, 0);
		}

		if (chn_idx == DSS_WCHN_W2) {
			hisifd->set_reg(hisifd, mctl_sys_base + MCTL_MOD20_DBG, 0xA0002, 32, 0);
		}
	}

	if (mctl_ch_base->chn_ov_en_base) {
		hisifd->set_reg(hisifd, mctl_ch_base->chn_ov_en_base, s_mctl_ch->chn_ov_oen, 32, 0);
	}

	hisifd->set_reg(hisifd, mctl_ch_base->chn_flush_en_base, s_mctl_ch->chn_flush_en, 32, 0);
}
/*lint +e715*/

void hisi_dss_mctl_mutex_lock(struct hisi_fb_data_type *hisifd,
	int ovl_idx)
{
	char __iomem *mctl_base = NULL;

	if (NULL == hisifd) {
		HISI_FB_ERR("hisifd is NULL");
		return;
	}
	if ((ovl_idx < DSS_OVL0) || (ovl_idx >= DSS_OVL_IDX_MAX)) {
		HISI_FB_ERR("ovl_idx is invalid");
		return;
	}

	mctl_base = hisifd->dss_module.mctl_base[ovl_idx];

	hisifd->set_reg(hisifd, mctl_base + MCTL_CTL_MUTEX, 0x1, 1, 0);
}

void hisi_dss_mctl_mutex_unlock(struct hisi_fb_data_type *hisifd,
	int ovl_idx)
{
	char __iomem *mctl_base = NULL;

	if (NULL == hisifd) {
		HISI_FB_ERR("hisifd is NULL");
		return;
	}
	if ((ovl_idx < DSS_OVL0) || (ovl_idx >= DSS_OVL_IDX_MAX)) {
		HISI_FB_ERR("ovl_idx is invalid");
		return;
	}

	mctl_base = hisifd->dss_module.mctl_base[ovl_idx];

	hisifd->set_reg(hisifd, mctl_base + MCTL_CTL_MUTEX, 0x0, 1, 0);
}

void hisi_dss_mctl_on(struct hisi_fb_data_type *hisifd, int mctl_idx, bool enable_cmdlist, bool fastboot_enable)
{
	char __iomem *mctl_base = NULL;
	char __iomem *mctl_sys_base = NULL;
	int i  = 0;
	int tmp = 0;

	if (NULL == hisifd) {
		HISI_FB_ERR("hisifd is NULL");
		return;
	}
	if ((mctl_idx < DSS_MCTL0) || (mctl_idx >= DSS_MCTL_IDX_MAX)) {
		HISI_FB_ERR("mctl_idx is invalid");
		return;
	}

	mctl_base = hisifd->dss_base +
		g_dss_module_ovl_base[mctl_idx][MODULE_MCTL_BASE];
	mctl_sys_base = hisifd->dss_base + DSS_MCTRL_SYS_OFFSET;

	set_reg(mctl_base + MCTL_CTL_EN, 0x1, 32, 0);

	if ((mctl_idx == DSS_MCTL0) || (mctl_idx == DSS_MCTL1)) {
		set_reg(mctl_base + MCTL_CTL_MUTEX_ITF, mctl_idx + 1, 32, 0);
	}


	if (enable_cmdlist) {
		tmp = MCTL_MOD_DBG_CH_NUM + MCTL_MOD_DBG_OV_NUM +
				MCTL_MOD_DBG_DBUF_NUM + MCTL_MOD_DBG_SCF_NUM;


		for (i = 0; i < tmp; i++) {
			set_reg(mctl_sys_base + MCTL_MOD0_DBG + i * 0x4, 0xA0000, 32, 0);//lint !e679
		}

		for (i = 0; i < MCTL_MOD_DBG_ITF_NUM; i++) {
			set_reg(mctl_sys_base + MCTL_MOD17_DBG + i * 0x4, 0xA0F00, 32, 0);//lint !e679
		}

		if (!fastboot_enable) {
			set_reg(mctl_base + MCTL_CTL_TOP, 0x1, 32, 0);
		}
	} else {
		set_reg(mctl_base + MCTL_CTL_DBG, 0xB13A00, 32, 0);

		if (is_mipi_cmd_panel(hisifd)) {
			set_reg(mctl_base + MCTL_CTL_TOP, 0x1, 32, 0);
		} else {
			if (mctl_idx == DSS_MCTL0) {
				set_reg(mctl_base + MCTL_CTL_TOP, 0x2, 32, 0);
			} else if (mctl_idx == DSS_MCTL1) {
				set_reg(mctl_base + MCTL_CTL_TOP, 0x3, 32, 0);
			} else {
				set_reg(mctl_base + MCTL_CTL_TOP, 0x1, 32, 0);
			}
		}
	}
}

void hisi_media_common_mctl_on(struct hisi_fb_data_type *hisifd)
{
	char __iomem *mctl_base = NULL;
	char __iomem *mctl_sys_base = NULL;

	if (NULL == hisifd) {
		HISI_FB_ERR("NULL ptr.\n");
		return;
	}

	mctl_base = hisifd->media_common_base + MCTL_MUTEX_OFFSET;
	mctl_sys_base = hisifd->media_common_base + MCTL_SYS_OFFSET;

	set_reg(mctl_base + MCTL_CTL_EN, 0x1, 32, 0);
	/*clear ISP configuration*/
	set_reg(mctl_base + MCTL_MUTEX_ISPIF, 0x0, 32, 0);
	set_reg(mctl_sys_base + MCTL_SYS_ISP_WCH_SEL, 0x0, 32, 0);

	set_reg(mctl_sys_base + MCTL_MOD2_DBG, 0xA8000, 32, 0);
	set_reg(mctl_sys_base + MCTL_MOD9_DBG, 0xA8000, 32, 0);

	set_reg(mctl_base + MCTL_CTL_TOP, 0x1, 32, 0);
}

int hisi_dss_mctl_ch_config(struct hisi_fb_data_type *hisifd, dss_overlay_t *pov_req,
	dss_layer_t *layer, dss_wb_layer_t *wb_layer, int ovl_idx, dss_rect_t *wb_ov_block_rect, bool has_base)
{
	int chn_idx = 0;
	int layer_idx = 0;
	dss_mctl_ch_t *mctl_ch = NULL;
	dss_mctl_sys_t *mctl_sys = NULL;
	int ch_ov_sel_pattern = 8;
	int chn_ov_sel_max_num = 7;

	if (NULL == hisifd) {
		HISI_FB_ERR("hisifd is NULL");
		return -EINVAL;
	}
	if (NULL == pov_req) {
		HISI_FB_ERR("pov_req is NULL");
		return -EINVAL;
	}
	if ((layer == NULL) && (wb_layer == NULL)) {
		HISI_FB_ERR("layer and wb_layer is NULL");
		return -EINVAL;
	}

	if (wb_layer) {
		chn_idx = wb_layer->chn_idx;

		mctl_sys = &(hisifd->dss_module.mctl_sys);
		hisifd->dss_module.mctl_sys_used = 1;

		mctl_ch = &(hisifd->dss_module.mctl_ch[chn_idx]);
		hisifd->dss_module.mctl_ch_used[chn_idx] = 1;

		if (pov_req->wb_compose_type == DSS_WB_COMPOSE_MEDIACOMMON) {
			mctl_ch->chn_ov_oen = set_bits32(mctl_ch->chn_ov_oen, 1, 32, 0);
		} else {
			if (chn_idx != DSS_WCHN_W2) {  // chicago copybit
				mctl_ch->chn_ov_oen = set_bits32(mctl_ch->chn_ov_oen,
					(ovl_idx - 1), 32, 0);

				if (pov_req->wb_layer_nums == MAX_DSS_DST_NUM) {
					mctl_sys->wchn_ov_sel[0] = set_bits32(mctl_sys->wchn_ov_sel[0], 3, 32, 0);
					mctl_sys->wch_ov_sel_used[0] = 1;
					mctl_sys->wchn_ov_sel[1] = set_bits32(mctl_sys->wchn_ov_sel[1], 3, 32, 0);
					mctl_sys->wch_ov_sel_used[1] = 1;
				} else {
					mctl_sys->wchn_ov_sel[ovl_idx - DSS_OVL2] =
						set_bits32(mctl_sys->wchn_ov_sel[ovl_idx - DSS_OVL2], (chn_idx - DSS_WCHN_W0 + 1), 32, 0);
					mctl_sys->wch_ov_sel_used[ovl_idx - DSS_OVL2] = 1;
				}
			}
		}

		mctl_ch->chn_mutex = set_bits32(mctl_ch->chn_mutex, 0x1, 1, 0);
		mctl_ch->chn_flush_en = set_bits32(mctl_ch->chn_flush_en, 0x1, 1, 0);
	} else if (layer) {
		chn_idx = layer->chn_idx;
		layer_idx = layer->layer_idx;

		if (layer->need_cap & CAP_BASE)
			return 0;

		if (has_base) {
			layer_idx -= 1;
			if (layer_idx < 0) {
				HISI_FB_ERR("fb%d, layer_idx(%d) is out of range!",
					hisifd->index, layer_idx);
				return -EINVAL;
			}
		}

		mctl_sys = &(hisifd->dss_module.mctl_sys);
		hisifd->dss_module.mctl_sys_used = 1;


		if (layer->need_cap & (CAP_DIM | CAP_PURE_COLOR)) {
			if (layer_idx < chn_ov_sel_max_num) {
				mctl_sys->chn_ov_sel[ovl_idx] = set_bits32(mctl_sys->chn_ov_sel[ovl_idx],
					ch_ov_sel_pattern, 4, (layer_idx + 1) * 4);
				mctl_sys->chn_ov_sel_used[ovl_idx] = 1;
			} else {
			}
		} else {
			mctl_ch = &(hisifd->dss_module.mctl_ch[chn_idx]);
			hisifd->dss_module.mctl_ch_used[chn_idx] = 1;

			mctl_ch->chn_mutex = set_bits32(mctl_ch->chn_mutex, 0x1, 1, 0);
			mctl_ch->chn_flush_en = set_bits32(mctl_ch->chn_flush_en, 0x1, 1, 0);


			if ((chn_idx != DSS_RCHN_V2)) {
				mctl_ch->chn_ov_oen = set_bits32(mctl_ch->chn_ov_oen,
					((1 << (layer_idx + 1)) | (0x100 << ovl_idx)), 32, 0);//lint !e701

				if (wb_ov_block_rect) {
					mctl_ch->chn_starty = set_bits32(mctl_ch->chn_starty,
						((layer->dst_rect.y - wb_ov_block_rect->y) | (0x8 << 16)), 32, 0);
				} else {
					mctl_ch->chn_starty = set_bits32(mctl_ch->chn_starty,
						(layer->dst_rect.y | (0x8 << 16)), 32, 0);
				}

				if (layer_idx < chn_ov_sel_max_num) {
					mctl_sys->chn_ov_sel[ovl_idx] = set_bits32(mctl_sys->chn_ov_sel[ovl_idx],
						chn_idx, 4, (layer_idx + 1) * 4);
					mctl_sys->chn_ov_sel_used[ovl_idx] = 1;
				}
			}
		}
	}

	return 0;
}

int hisi_dss_mctl_ov_config(struct hisi_fb_data_type *hisifd, dss_overlay_t *pov_req,
	int ovl_idx, bool has_base, bool is_first_ov_block)
{
	dss_mctl_t *mctl = NULL;
	dss_mctl_sys_t *mctl_sys = NULL;

	if (NULL == hisifd) {
		HISI_FB_ERR("hisifd is NULL");
		return -EINVAL;
	}
	if ((ovl_idx < DSS_OVL0) || (ovl_idx >= DSS_OVL_IDX_MAX)) {
		HISI_FB_ERR("ovl_idx is invalid");
		return -EINVAL;
	}

	if (pov_req && pov_req->wb_layer_infos[0].chn_idx == DSS_WCHN_W2) { //chicago copybit no ovl
		return 0;
	}

	// MCTL_MUTEX
	mctl = &(hisifd->dss_module.mctl[ovl_idx]);
	hisifd->dss_module.mctl_used[ovl_idx] = 1;

	if (ovl_idx == DSS_OVL0) {
		mctl->ctl_mutex_itf = set_bits32(mctl->ctl_mutex_itf, 0x1, 2, 0);
		mctl->ctl_mutex_dbuf = set_bits32(mctl->ctl_mutex_dbuf, 0x1, 2, 0);
	} else if (ovl_idx == DSS_OVL1) {
		mctl->ctl_mutex_itf = set_bits32(mctl->ctl_mutex_itf, 0x2, 2, 0);
		mctl->ctl_mutex_dbuf = set_bits32(mctl->ctl_mutex_dbuf, 0x2, 2, 0);
	} else {
		;
	}

	mctl->ctl_mutex_ov= set_bits32(mctl->ctl_mutex_ov, 1 << ovl_idx, 4, 0);

	// MCTL_SYS
	mctl_sys = &(hisifd->dss_module.mctl_sys);
	hisifd->dss_module.mctl_sys_used = 1;

	// ov base pattern
	mctl_sys->chn_ov_sel[ovl_idx] = set_bits32(mctl_sys->chn_ov_sel[ovl_idx], 0x8, 4, 0);
	mctl_sys->chn_ov_sel_used[ovl_idx] = 1;

	if ((ovl_idx == DSS_OVL0) || (ovl_idx == DSS_OVL1)) {
		if (is_first_ov_block) {
			mctl_sys->ov_flush_en[ovl_idx] = set_bits32(mctl_sys->ov_flush_en[ovl_idx], 0xd, 4, 0);
		} else {
			mctl_sys->ov_flush_en[ovl_idx] = set_bits32(mctl_sys->ov_flush_en[ovl_idx], 0x1, 1, 0);
		}
		mctl_sys->ov_flush_en_used[ovl_idx] = 1;
	} else {
		mctl_sys->ov_flush_en[ovl_idx] = set_bits32(mctl_sys->ov_flush_en[ovl_idx], 0x1, 1, 0);
		mctl_sys->ov_flush_en_used[ovl_idx] = 1;
	}

	return 0;
}


/*******************************************************************************
** DSS OVL
*/
static dss_ovl_alpha_t g_ovl_alpha[DSS_BLEND_MAX] = {
	{0,0,0,0,0,  0,  0,0,0,0,  0},	//DSS_BLEND_CLEAR
	{0,0,0,0,0,  0,  1,0,0,0,  0},	//DSS_BLEND_SRC
	{1,0,0,0,0,  0,  0,0,0,0,  0},	//DSS_BLEND_DST
	{3,0,0,0,1,  0,  1,0,0,0,  0},	//DSS_BLEND_SRC_OVER_DST
	{1,0,0,0,0,  0,  3,0,0,1,  0},	//DSS_BLEND_DST_OVER_SRC
	{0,0,0,0,0,  0,  3,0,0,0,  0},	//DSS_BLEND_SRC_IN_DST
	{3,0,0,0,0,  0,  0,0,0,0,  0},	//DSS_BLEND_DST_IN_SRC
	{0,0,0,0,0,  0,  3,0,0,1,  0},	//DSS_BLEND_SRC_OUT_DST
	{3,0,0,0,1,  0,  0,0,0,0,  0},	//DSS_BLEND_DST_OUT_SRC
	{3,0,0,0,1,  0,  3,0,0,0,  0},	//DSS_BLEND_SRC_ATOP_DST
	{3,0,0,0,0,  0,  3,0,0,1,  0},	//DSS_BLEND_DST_ATOP_SRC
	{3,0,0,0,1,  0,  3,0,0,1,  0},	//DSS_BLEND_SRC_XOR_DST
	{1,0,0,0,0,  0,  1,0,0,0,  0},	//DSS_BLEND_SRC_ADD_DST
	{3,0,0,0,1,  0,  3,0,0,0,  1},	//DSS_BLEND_FIX_OVER
	{3,0,0,0,1,  0,  3,2,0,0,  0},	//DSS_BLEND_FIX_PER0
	{3,0,0,0,1,  1,  3,1,0,0,  1},	//DSS_BLEND_FIX_PER1
	{2,2,0,0,0,  0,  1,0,0,0,  0},	//DSS_BLEND_FIX_PER2
	{1,0,0,0,0,  0,  2,2,0,0,  0},	//DSS_BLEND_FIX_PER3
	{0,0,0,0,0,  0,  3,2,0,0,  0},	//DSS_BLEND_FIX_PER4
	{3,2,0,0,0,  0,  0,0,0,0,  0},	//DSS_BLEND_FIX_PER5
	{0,0,0,0,0,  0,  2,2,0,0,  0},	//DSS_BLEND_FIX_PER6
	{2,2,0,0,0,  0,  0,0,0,0,  0},	//DSS_BLEND_FIX_PER7
	{2,2,0,0,0,  0,  3,2,0,0,  0},	//DSS_BLEND_FIX_PER8
	{3,2,0,0,0,  0,  2,2,0,0,  0},	//DSS_BLEND_FIX_PER9
	{2,2,0,0,0,  0,  2,2,0,0,  0},	//DSS_BLEND_FIX_PER10
	{3,2,0,0,0,  0,  3,2,0,0,  0},	//DSS_BLEND_FIX_PER11
	{2,1,0,0,0,  0,  3,2,0,0,  0},	//DSS_BLEND_FIX_PER12 DSS_BLEND_SRC_OVER_DST
	{2,1,0,0,0,  0,  3,1,0,0,  1},	//DSS_BLEND_FIX_PER13 DSS_BLEND_FIX_OVER
	{0,0,0,0,0,  1,  3,0,1,0,  0},	//DSS_BLEND_FIX_PER14 BASE_FF

	{2,1,0,0,0,  1,  3,2,0,0,  0},	//DSS_BLEND_FIX_PER15 DSS_BLEND_SRC_OVER_DST
	{2,1,0,0,0,  1,  3,1,0,0,  1},	//DSS_BLEND_FIX_PER16 DSS_BLEND_FIX_OVER
	{0,0,0,0,0,  1,  1,0,0,0,  0},	//DSS_BLEND_FIX_PER17 DSS_BLEND_SRC
};

static uint32_t get_ovl_blending_mode(dss_overlay_t *pov_req, dss_layer_t *layer)
{
	uint32_t blend_mode = 0;
	bool has_per_pixel_alpha = false;

	if (NULL == layer) {
		HISI_FB_ERR("layer is NULL");
		return EINVAL;
	}

	has_per_pixel_alpha = hal_format_has_alpha(layer->img.format);

	/*
	if (layer->layer_idx == 0) {
		if (has_per_pixel_alpha) {
			blend_mode = DSS_BLEND_SRC;
		} else {
			blend_mode= DSS_BLEND_FIX_PER17;
		}
	} else
	*/
	{
		if (layer->blending == HISI_FB_BLENDING_PREMULT) {
			if (has_per_pixel_alpha) {
				blend_mode = (layer->glb_alpha < 0xFF) ? DSS_BLEND_FIX_PER12 : DSS_BLEND_SRC_OVER_DST;
			} else {
				//if (layer->need_cap & (CAP_DIM | CAP_PURE_COLOR))
				blend_mode = (layer->glb_alpha < 0xFF) ? DSS_BLEND_FIX_PER8 : DSS_BLEND_SRC;
			}
		} else if (layer->blending == HISI_FB_BLENDING_COVERAGE) {
			if (has_per_pixel_alpha) {
				blend_mode = (layer->glb_alpha < 0xFF) ? DSS_BLEND_FIX_PER13 : DSS_BLEND_FIX_OVER;
			} else {
				blend_mode = (layer->glb_alpha < 0xFF) ? DSS_BLEND_FIX_PER8 : DSS_BLEND_SRC;
			}
		} else {
			if (has_per_pixel_alpha) {
				blend_mode = DSS_BLEND_SRC;
			} else {
				blend_mode = DSS_BLEND_FIX_PER17;
			}
		}
	}

	if (g_debug_ovl_online_composer || g_debug_ovl_offline_composer) {
		HISI_FB_INFO("layer_idx(%d), blending=%d, fomat=%d, has_per_pixel_alpha=%d, blend_mode=%d.\n",
			layer->layer_idx, layer->blending, layer->img.format, has_per_pixel_alpha, blend_mode);
	}

	return blend_mode;
}

int hisi_dss_ovl_base_config(struct hisi_fb_data_type *hisifd, dss_overlay_t *pov_req,
	  dss_overlay_block_t *pov_h_block, dss_rect_t *wb_ov_block_rect, int ovl_idx, int ov_h_block_idx)
{
	dss_ovl_t *ovl = NULL;
	int img_width = 0;
	int img_height = 0;
	int block_size = 0x7FFF;
	int temp = 0;
	int i = 0;
	int m = 0;
	uint32_t ovl_bg_color;

	dss_overlay_block_t *pov_h_block_infos_tmp = NULL;
	dss_overlay_block_t *pov_h_block_tmp = NULL;
	dss_layer_t *layer = NULL;
	int pov_h_block_idx = 0;
	int layer_idx = 0;
	bool has_base = false;

	if (NULL == hisifd) {
		HISI_FB_ERR("hisifd is NULL");
		return -EINVAL;
	}
	if ((ovl_idx < DSS_OVL0) || (ovl_idx >= DSS_OVL_IDX_MAX)) {
		HISI_FB_ERR("ovl_idx is invalid");
		return -EINVAL;
	}

	if (pov_req && pov_req->wb_layer_infos[0].chn_idx == DSS_WCHN_W2) { //chicago copybit no ovl
		return 0;
	}

	ovl = &(hisifd->dss_module.ov[ovl_idx]);
	hisifd->dss_module.ov_used[ovl_idx] = 1;

	if (wb_ov_block_rect) {
		img_width = wb_ov_block_rect->w;
		img_height = wb_ov_block_rect->h;
	} else {
		if ((!pov_req) || (pov_req->dirty_rect.x == 0 && pov_req->dirty_rect.y == 0 &&
			pov_req->dirty_rect.w == 0 && pov_req->dirty_rect.h == 0)) {
			img_width = get_panel_xres(hisifd);
			img_height = get_panel_yres(hisifd);
		} else {
			img_width = pov_req->dirty_rect.w;
			img_height = pov_req->dirty_rect.h;
		}
	}

	if (pov_h_block && pov_req && (pov_req->ov_block_nums != 0)) {
		if (pov_req->ov_block_nums > 1) {
			pov_h_block_infos_tmp = (dss_overlay_block_t *)(pov_req->ov_block_infos_ptr);
			for (m = ov_h_block_idx; m < (pov_req->ov_block_nums); m++) {
				pov_h_block_tmp = &(pov_h_block_infos_tmp[m]);
				has_base = false;

				for (i = 0; i < (pov_h_block_tmp->layer_nums); i++) {
					layer = &(pov_h_block_tmp->layer_infos[i]);
					//layer_idx = layer->layer_idx;

					if (layer->need_cap & CAP_BASE) {
						HISI_FB_INFO("layer%d is base, i=%d!\n", layer->layer_idx, i);
						has_base = true;
						continue;
					}

					layer_idx = i;
					if (has_base) {
						layer_idx = i - 1;
					}

					//HISI_FB_INFO("m=%d, layer_idx=%d,  layer->layer_idx=%d, dst_rect(%d,%d,%d,%d)\n",
					//	m, i,  layer->layer_idx, layer->dst_rect.x, layer->dst_rect.y, layer->dst_rect.w, layer->dst_rect.y);
					if (layer_idx >= pov_h_block_idx) {
						ovl->ovl_layer[layer_idx].layer_pos = set_bits32(ovl->ovl_layer[layer_idx].layer_pos, 0, 15, 0);
						ovl->ovl_layer[layer_idx].layer_pos = set_bits32(ovl->ovl_layer[layer_idx].layer_pos, img_height, 15, 16);

						ovl->ovl_layer[layer_idx].layer_size = set_bits32(ovl->ovl_layer[layer_idx].layer_size, img_width, 15, 0);
						ovl->ovl_layer[layer_idx].layer_size = set_bits32(ovl->ovl_layer[layer_idx].layer_size, img_height + 1, 15, 16);
						ovl->ovl_layer[layer_idx].layer_cfg = set_bits32(ovl->ovl_layer[layer_idx].layer_cfg, 0x1, 1, 0);

						if (layer->need_cap & (CAP_DIM | CAP_PURE_COLOR)) {
							ovl->ovl_layer[layer_idx].layer_pattern =
								set_bits32(ovl->ovl_layer[layer_idx].layer_pattern, layer->color, 32, 0);
							ovl->ovl_layer[layer_idx].layer_cfg =
								set_bits32(ovl->ovl_layer[layer_idx].layer_cfg, 1, 1, 0);
							ovl->ovl_layer[layer_idx].layer_cfg =
								set_bits32(ovl->ovl_layer[layer_idx].layer_cfg, 1, 1, 8);
						} else {
							ovl->ovl_layer[layer_idx].layer_pattern =
								set_bits32(ovl->ovl_layer[layer_idx].layer_pattern, 0, 32, 0);
							ovl->ovl_layer[layer_idx].layer_cfg =
								set_bits32(ovl->ovl_layer[layer_idx].layer_cfg, 1, 1, 0);
							ovl->ovl_layer[layer_idx].layer_cfg =
								set_bits32(ovl->ovl_layer[layer_idx].layer_cfg, 0, 1, 8);
						}

						ovl->ovl_layer_used[layer_idx] = 1;
						pov_h_block_idx = layer_idx + 1;
					}
				}
			}
		}

		if (wb_ov_block_rect) {
			if ((pov_req->wb_layer_infos[0].transform & HISI_FB_TRANSFORM_ROT_90)
				|| (pov_req->wb_layer_infos[1].transform & HISI_FB_TRANSFORM_ROT_90)) {
				block_size = DSS_HEIGHT(wb_ov_block_rect->h);
			} else {
				temp = pov_h_block->ov_block_rect.y + DSS_HEIGHT(pov_h_block->ov_block_rect.h) - wb_ov_block_rect->y;
				if (temp >= wb_ov_block_rect->h) {
					block_size = DSS_HEIGHT(wb_ov_block_rect->h);
				} else {
					block_size = temp;
				}
			}
		} else {
			block_size = pov_h_block->ov_block_rect.y + DSS_HEIGHT(pov_h_block->ov_block_rect.h);
		}
	}

	ovl->ovl_size = set_bits32(ovl->ovl_size, DSS_WIDTH(img_width), 15, 0);
	ovl->ovl_size = set_bits32(ovl->ovl_size, DSS_HEIGHT(img_height), 15, 16);

		ovl_bg_color = 0xFF000000;
	ovl->ovl_bg_color= set_bits32(ovl->ovl_bg_color, ovl_bg_color, 32, 0);


	ovl->ovl_dst_startpos = set_bits32(ovl->ovl_dst_startpos, 0x0, 32, 0);
	ovl->ovl_dst_endpos = set_bits32(ovl->ovl_dst_endpos, DSS_WIDTH(img_width), 15, 0);
	ovl->ovl_dst_endpos = set_bits32(ovl->ovl_dst_endpos, DSS_HEIGHT(img_height), 15, 16);
	ovl->ovl_gcfg = set_bits32(ovl->ovl_gcfg, 0x1, 1, 0);
	ovl->ovl_gcfg = set_bits32(ovl->ovl_gcfg, 0x1, 1, 16);
	ovl->ovl_block_size = set_bits32(ovl->ovl_block_size, block_size, 15, 16);

	return 0;
}

/*lint -e676 -e644*/
int hisi_dss_ovl_layer_config(struct hisi_fb_data_type *hisifd, dss_overlay_t *pov_req,
	dss_layer_t *layer, dss_rect_t *wb_ov_block_rect, bool has_base)
{
	dss_ovl_t *ovl = NULL;
	int ovl_idx = 0;
	int layer_idx = 0;
	int blend_mode = 0;
	dss_rect_t wb_ov_rect;
	dss_rect_t dst_rect;
	uint32_t color_rgb;

	if (hisifd == NULL) {
		HISI_FB_ERR("hisifd is NULL Point.\n");
		return -EINVAL;
	}

	if (layer == NULL) {
		HISI_FB_ERR("layer is NULL Point.\n");
		return -EINVAL;
	}

	ovl_idx = hisifd->ov_req.ovl_idx;
	layer_idx = layer->layer_idx;

	if (g_dss_version_tag == FB_ACCEL_HI366x) {
		if (layer->chn_idx == DSS_RCHN_V2) { //chicago copybit
			return 0;
		}
	}

	if (hisifd->index == MEDIACOMMON_PANEL_IDX) {
		return 0;
	}

	if ((ovl_idx < DSS_OVL0) || (ovl_idx >= DSS_OVL_IDX_MAX)) {
		HISI_FB_ERR("ovl_idx = %d is out of range!\n", ovl_idx);
		return -EINVAL;
	}

	ovl = &(hisifd->dss_module.ov[ovl_idx]);
	hisifd->dss_module.ov_used[ovl_idx] = 1;

	if (layer->need_cap & CAP_BASE) {
		color_rgb = layer->color;
		ovl->ovl_bg_color = set_bits32(ovl->ovl_bg_color, color_rgb, 32, 0);
		ovl->ovl_gcfg = set_bits32(ovl->ovl_gcfg, 0x1, 1, 16);
		return 0;
	}

	if ((layer->glb_alpha) < 0) {
		HISI_FB_ERR("layer's glb_alpha(0x%x) is out of range!", layer->glb_alpha);
		layer->glb_alpha = 0;
	} else if ((layer->glb_alpha) > 0xFF) {
		HISI_FB_ERR("layer's glb_alpha(0x%x) is out of range!", layer->glb_alpha);
		layer->glb_alpha = 0xFF;
	}

	color_rgb = layer->color;

	blend_mode = get_ovl_blending_mode(pov_req, layer);
	if ((blend_mode < 0) || (blend_mode >= DSS_BLEND_MAX)) {
		HISI_FB_ERR("blend_mode = %d is out of range!\n", blend_mode);
		return -EINVAL;
	}

	if (has_base) {
		layer_idx -= 1;
		if (layer_idx < 0) {
			HISI_FB_ERR("layer_idx(%d) is out of range!\n", layer_idx);
			return -EINVAL;
		}
	}

	ovl->ovl_layer_used[layer_idx] = 1;
	if ((layer->chn_idx == DSS_RCHN_V0) && layer->block_info.arsr2p_left_clip) {
		dst_rect.x = layer->dst_rect.x + layer->block_info.arsr2p_left_clip;
		dst_rect.y = layer->dst_rect.y;
		dst_rect.w = layer->dst_rect.w - layer->block_info.arsr2p_left_clip;
		dst_rect.h = layer->dst_rect.h;
	} else {
		dst_rect = layer->dst_rect;
	}

	if (wb_ov_block_rect) {
		wb_ov_rect.x = pov_req->wb_ov_rect.x + wb_ov_block_rect->x;
		wb_ov_rect.y = pov_req->wb_ov_rect.y;

		ovl->ovl_layer[layer_idx].layer_pos = set_bits32(ovl->ovl_layer[layer_idx].layer_pos,
			(dst_rect.x - wb_ov_rect.x), 15, 0);
		ovl->ovl_layer[layer_idx].layer_pos = set_bits32(ovl->ovl_layer[layer_idx].layer_pos,
			(dst_rect.y - wb_ov_rect.y), 15, 16);

		ovl->ovl_layer[layer_idx].layer_size = set_bits32(ovl->ovl_layer[layer_idx].layer_size,
			(dst_rect.x - wb_ov_rect.x + DSS_WIDTH(dst_rect.w)), 15, 0);
		ovl->ovl_layer[layer_idx].layer_size = set_bits32(ovl->ovl_layer[layer_idx].layer_size,
			(dst_rect.y - wb_ov_rect.y + DSS_HEIGHT(dst_rect.h)), 15, 16);
	} else {
		ovl->ovl_layer[layer_idx].layer_pos = set_bits32(ovl->ovl_layer[layer_idx].layer_pos,
			dst_rect.x, 15, 0);
		ovl->ovl_layer[layer_idx].layer_pos = set_bits32(ovl->ovl_layer[layer_idx].layer_pos,
			dst_rect.y, 15, 16);

		ovl->ovl_layer[layer_idx].layer_size = set_bits32(ovl->ovl_layer[layer_idx].layer_size,
			DSS_WIDTH(dst_rect.x + dst_rect.w), 15, 0);
		ovl->ovl_layer[layer_idx].layer_size = set_bits32(ovl->ovl_layer[layer_idx].layer_size,
			DSS_HEIGHT(dst_rect.y + dst_rect.h), 15, 16);
	}

	ovl->ovl_layer[layer_idx].layer_alpha = set_bits32(ovl->ovl_layer[layer_idx].layer_alpha,
		((layer->glb_alpha << 0) |
		(g_ovl_alpha[blend_mode].fix_mode << 8) |
		(g_ovl_alpha[blend_mode].dst_pmode << 9) | (g_ovl_alpha[blend_mode].alpha_offdst << 10) |
		(g_ovl_alpha[blend_mode].dst_gmode << 12) | (g_ovl_alpha[blend_mode].dst_amode << 14) |
		(layer->glb_alpha << 16) |
		(g_ovl_alpha[blend_mode].alpha_smode << 24) |
		(g_ovl_alpha[blend_mode].src_pmode << 25) | (g_ovl_alpha[blend_mode].src_lmode << 26) |
		(g_ovl_alpha[blend_mode].alpha_offdst << 27) | (g_ovl_alpha[blend_mode].src_gmode << 28) |
		(g_ovl_alpha[blend_mode].src_amode << 30)), 32, 0);

	if (layer->need_cap & (CAP_DIM | CAP_PURE_COLOR)) {
		ovl->ovl_layer[layer_idx].layer_pattern =
			set_bits32(ovl->ovl_layer[layer_idx].layer_pattern, color_rgb, 32, 0);
		ovl->ovl_layer[layer_idx].layer_cfg =
			set_bits32(ovl->ovl_layer[layer_idx].layer_cfg, 0x1, 1, 0);
		ovl->ovl_layer[layer_idx].layer_cfg =
			set_bits32(ovl->ovl_layer[layer_idx].layer_cfg, 0x1, 1, 8);
	} else {
		ovl->ovl_layer[layer_idx].layer_pattern =
			set_bits32(ovl->ovl_layer[layer_idx].layer_pattern, 0x0, 32, 0);
		ovl->ovl_layer[layer_idx].layer_cfg =
			set_bits32(ovl->ovl_layer[layer_idx].layer_cfg, 0x1, 1, 0);
		ovl->ovl_layer[layer_idx].layer_cfg =
			set_bits32(ovl->ovl_layer[layer_idx].layer_cfg, 0x0, 1, 8);
	}

	ovl->ovl_layer_pos[layer_idx].layer_pspos =
		set_bits32(ovl->ovl_layer_pos[layer_idx].layer_pspos, dst_rect.x, 15, 0);
	ovl->ovl_layer_pos[layer_idx].layer_pspos =
		set_bits32(ovl->ovl_layer_pos[layer_idx].layer_pspos, dst_rect.y, 15, 16);
	ovl->ovl_layer_pos[layer_idx].layer_pepos =
		set_bits32(ovl->ovl_layer_pos[layer_idx].layer_pepos,
		DSS_WIDTH(dst_rect.x + dst_rect.w), 15, 0);
	ovl->ovl_layer_pos[layer_idx].layer_pepos =
		set_bits32(ovl->ovl_layer_pos[layer_idx].layer_pepos,
		DSS_HEIGHT(dst_rect.y + dst_rect.h), 15, 16);

	return 0;
}
/*lint +e676 +e644*/

/*******************************************************************************
** dirty_region_updt
*/
static void hisi_dss_dirty_region_dbuf_set_reg(struct hisi_fb_data_type *hisifd,
	char __iomem *dss_base, dirty_region_updt_t *s_dirty_region_updt)
{
	if (hisifd == NULL) {
		HISI_FB_DEBUG("hisifd is NULL!\n");
		return;
	}

	if (dss_base == NULL) {
		HISI_FB_DEBUG("dss_base is NULL!\n");
		return;
	}

	if (s_dirty_region_updt == NULL) {
		HISI_FB_DEBUG("s_dirty_region_updt is NULL!\n");
		return;
	}

	hisifd->set_reg(hisifd, dss_base + DSS_DBUF0_OFFSET + DBUF_FRM_SIZE,
		s_dirty_region_updt->dbuf_frm_size, 32, 0);
	hisifd->set_reg(hisifd, dss_base + DSS_DBUF0_OFFSET + DBUF_FRM_HSIZE,
		s_dirty_region_updt->dbuf_frm_hsize, 32, 0);

	hisifd->set_reg(hisifd, dss_base + DSS_DPP_OFFSET + DPP_IMG_SIZE_BEF_SR,
		s_dirty_region_updt->dpp_img_size_bef_sr, 32, 0);
	hisifd->set_reg(hisifd, dss_base + DSS_DPP_OFFSET + DPP_IMG_SIZE_AFT_SR,
		s_dirty_region_updt->dpp_img_size_aft_sr, 32, 0);
}

static void hisi_dss_dirty_region_updt_set_reg(struct hisi_fb_data_type *hisifd,
	char __iomem *dss_base, dirty_region_updt_t *s_dirty_region_updt)
{
	if (NULL == hisifd) {
		HISI_FB_ERR("hisifd is NULL");
		return;
	}
	if (NULL == dss_base) {
		HISI_FB_ERR("dss_base is NULL");
		return;
	}
	if (NULL == s_dirty_region_updt) {
		HISI_FB_ERR("s_dirty_region_updt is NULL");
		return;
	}

	set_reg(dss_base + DSS_MIPI_DSI0_OFFSET + MIPIDSI_EDPI_CMD_SIZE_OFFSET,
		s_dirty_region_updt->edpi_cmd_size, 32, 0);
	if (is_dual_mipi_panel(hisifd)) {
		set_reg(dss_base + DSS_MIPI_DSI1_OFFSET + MIPIDSI_EDPI_CMD_SIZE_OFFSET,
			s_dirty_region_updt->edpi_cmd_size, 32, 0);
	}

	set_reg(dss_base + DSS_LDI0_OFFSET + LDI_DPI0_HRZ_CTRL0,
		s_dirty_region_updt->ldi_dpi0_hrz_ctrl0, 29, 0);
	set_reg(dss_base + DSS_LDI0_OFFSET + LDI_DPI0_HRZ_CTRL1,
		s_dirty_region_updt->ldi_dpi0_hrz_ctrl1, 13, 0);
	set_reg(dss_base + DSS_LDI0_OFFSET + LDI_DPI0_HRZ_CTRL2,
		s_dirty_region_updt->ldi_dpi0_hrz_ctrl2, 13, 0);
	set_reg(dss_base + DSS_LDI0_OFFSET + LDI_VRT_CTRL0,
		s_dirty_region_updt->ldi_vrt_ctrl0, 29, 0);
	set_reg(dss_base + DSS_LDI0_OFFSET + LDI_VRT_CTRL1,
		s_dirty_region_updt->ldi_vrt_ctrl1, 13, 0);
	set_reg(dss_base + DSS_LDI0_OFFSET + LDI_VRT_CTRL2,
		s_dirty_region_updt->ldi_vrt_ctrl2, 13, 0);

	if (is_dual_mipi_panel(hisifd)) {
		set_reg(dss_base + DSS_LDI0_OFFSET + LDI_DPI1_HRZ_CTRL0,
			s_dirty_region_updt->ldi_dpi1_hrz_ctrl0, 29, 0);
		set_reg(dss_base + DSS_LDI0_OFFSET + LDI_DPI1_HRZ_CTRL1,
			s_dirty_region_updt->ldi_dpi1_hrz_ctrl1, 13, 0);
		set_reg(dss_base + DSS_LDI0_OFFSET + LDI_DPI1_HRZ_CTRL2,
			s_dirty_region_updt->ldi_dpi1_hrz_ctrl2, 13, 0);
	}

	if (hisifd->panel_info.ifbc_type != IFBC_TYPE_NONE) {
		if (!is_ifbc_vesa_panel(hisifd)) {
			set_reg(dss_base + DSS_IFBC_OFFSET + IFBC_SIZE,
				s_dirty_region_updt->ifbc_size, 32, 0);
		} else {
			set_reg(dss_base + DSS_DSC_OFFSET + DSC_PIC_SIZE,
				s_dirty_region_updt->ifbc_size, 32, 0);
		}
	}

}

int hisi_dss_dirty_region_dbuf_config(struct hisi_fb_data_type *hisifd,
	dss_overlay_t *pov_req)
{
	struct hisi_panel_info *pinfo = NULL;
	dirty_region_updt_t *dirty_region_updt = NULL;
	struct dss_rect dirty = {0};

	if (NULL == hisifd) {
		HISI_FB_ERR("hisifd is NULL");
		return -EINVAL;
	}
	pinfo = &(hisifd->panel_info);

	if ((hisifd->index != PRIMARY_PANEL_IDX) ||
		!pinfo->dirty_region_updt_support)
		return 0;

	if ((!pov_req) || (pov_req->dirty_rect.x == 0 && pov_req->dirty_rect.y == 0 &&
		pov_req->dirty_rect.w == 0 && pov_req->dirty_rect.h == 0)) {
		dirty.x = 0;
		dirty.y = 0;
		dirty.w = hisifd->panel_info.xres;
		dirty.h = hisifd->panel_info.yres;
	} else {
		dirty = pov_req->dirty_rect;
	}

	if ((dirty.x == hisifd->dirty_region_updt.x)
		&& (dirty.y == hisifd->dirty_region_updt.y)
		&& (dirty.w == hisifd->dirty_region_updt.w)
		&& (dirty.h == hisifd->dirty_region_updt.h)) {
		return 0;
	}

	dirty_region_updt = &(hisifd->dss_module.dirty_region_updt);
	hisifd->dss_module.dirty_region_updt_used = 1;

	dirty_region_updt->dpp_img_size_bef_sr = set_bits32(dirty_region_updt->dpp_img_size_bef_sr,
		(DSS_WIDTH((uint32_t)dirty.h) << 16) | DSS_WIDTH((uint32_t)dirty.w), 32, 0);
	dirty_region_updt->dpp_img_size_aft_sr= set_bits32(dirty_region_updt->dpp_img_size_aft_sr,
		(DSS_WIDTH((uint32_t)dirty.h) << 16) | DSS_WIDTH((uint32_t)dirty.w), 32, 0);

	dirty_region_updt->dbuf_frm_size = set_bits32(dirty_region_updt->dbuf_frm_size,
		dirty.w * dirty.h, 27, 0);
	dirty_region_updt->dbuf_frm_hsize = set_bits32(dirty_region_updt->dbuf_frm_hsize,
		DSS_WIDTH(dirty.w), 13, 0);

	return 0;
}

void hisi_dss_dirty_region_updt_config(struct hisi_fb_data_type *hisifd,
	dss_overlay_t *pov_req)
{
	struct hisi_fb_panel_data *pdata = NULL;
	struct hisi_panel_info *pinfo = NULL;
	dirty_region_updt_t *dirty_region_updt = NULL;
	struct dss_rect dirty = {0};
	uint32_t h_porch_pading = 0;
	uint32_t v_porch_pading = 0;
	dss_rect_t rect = {0};
	uint32_t max_latency = 0;
	uint32_t bits_per_pixel = 0;
	uint32_t h_front_porch_max = 0;
	uint32_t h_front_porch = 0;
	uint32_t h_back_porch= 0;

	if (NULL == hisifd) {
		HISI_FB_ERR("hisifd is NULL");
		return;
	}
	pdata = dev_get_platdata(&hisifd->pdev->dev);
	if (NULL == pdata) {
		HISI_FB_ERR("pdata is NULL");
		return;
	}
	pinfo = &(hisifd->panel_info);

	if ((hisifd->index != PRIMARY_PANEL_IDX) ||
		!pinfo->dirty_region_updt_support)
		return ;

	if ((!pov_req) || (pov_req->dirty_rect.w == 0) || (pov_req->dirty_rect.h == 0)) {
		dirty.x = 0;
		dirty.y = 0;
		dirty.w = hisifd->panel_info.xres;
		dirty.h = hisifd->panel_info.yres;
	} else {
		dirty = pov_req->dirty_rect;
	}

	if ((hisifd->panel_info.xres) >= dirty.w) {
		h_porch_pading = hisifd->panel_info.xres - dirty.w;
	}

	if (hisifd->panel_info.yres >= dirty.h) {
		v_porch_pading = hisifd->panel_info.yres - dirty.h;
	}

	if ((dirty.x == hisifd->dirty_region_updt.x)
		&& (dirty.y == hisifd->dirty_region_updt.y)
		&& (dirty.w == hisifd->dirty_region_updt.w)
		&& (dirty.h == hisifd->dirty_region_updt.h)) {
		return ;
	}

	rect.x = 0;
	rect.y = 0;
	rect.w = dirty.w;
	rect.h = dirty.h;
	mipi_ifbc_get_rect(hisifd, &rect);

	h_front_porch = pinfo->ldi.h_front_porch;
	h_back_porch = pinfo->ldi.h_back_porch;

	h_porch_pading = h_porch_pading * rect.w / dirty.w;
	v_porch_pading = v_porch_pading * rect.h / dirty.h;

	// max_latency = data_lp2hs_time + (EDPI_CMD_SIZE * bytes_per_pixel + 1 + 6) / number_of_lanes
	// h_front_porch <= max_latency * byte_clk_cycle / pixel_clk_cycle
	if (pinfo->bpp == LCD_RGB888)
		bits_per_pixel = 24;
	else if (pinfo->bpp == LCD_RGB565)
		bits_per_pixel = 16;
	else
		bits_per_pixel = 24;

	if (pinfo->pxl_clk_rate_div == 0)
		pinfo->pxl_clk_rate_div = 1;

	max_latency = (rect.w * bits_per_pixel / 8 + 1 + 6) / (pinfo->mipi.lane_nums + 1);

	if (pinfo->mipi.phy_mode == DPHY_MODE) {
		if (pinfo->dsi_phy_ctrl.lane_byte_clk != 0) {
			h_front_porch_max = max_latency * (pinfo->pxl_clk_rate / pinfo->pxl_clk_rate_div) / pinfo->dsi_phy_ctrl.lane_byte_clk;
		}
	} else if (pinfo->mipi.phy_mode == CPHY_MODE) {
		if (pinfo->dsi_phy_ctrl.lane_word_clk != 0) {
			h_front_porch_max = max_latency * (pinfo->pxl_clk_rate / pinfo->pxl_clk_rate_div) / pinfo->dsi_phy_ctrl.lane_word_clk;
		}
	}

	HISI_FB_DEBUG("bits_per_pixel = %d\n"
		"data_lane_lp2hs_time = %d\n"
		"max_latency = %d\n"
		"pxl_clk_rate = %lld\n"
		"pxl_clk_rate_div = %d\n"
		"dsi_phy_ctrl.lane_byte_clk = %lld\n"
		"h_front_porch_max = %d\n",
		bits_per_pixel,
		pinfo->dsi_phy_ctrl.data_lane_lp2hs_time,
		max_latency,
		pinfo->pxl_clk_rate,
		pinfo->pxl_clk_rate_div,
		pinfo->dsi_phy_ctrl.lane_byte_clk,
		h_front_porch_max);

	if (h_front_porch > h_front_porch_max) {
		h_back_porch += (h_front_porch - h_front_porch_max);
		h_front_porch = h_front_porch_max;
	}

	dirty_region_updt = &(hisifd->dss_module.dirty_region_updt);

	dirty_region_updt->ldi_dpi0_hrz_ctrl0 = set_bits32(dirty_region_updt->ldi_dpi0_hrz_ctrl0,
		(h_front_porch) | ((h_back_porch + h_porch_pading) << 16), 29, 0);
	dirty_region_updt->ldi_dpi0_hrz_ctrl1 = set_bits32(dirty_region_updt->ldi_dpi0_hrz_ctrl1,
		DSS_WIDTH(pinfo->ldi.h_pulse_width), 13, 0);
	dirty_region_updt->ldi_dpi0_hrz_ctrl2 = set_bits32(dirty_region_updt->ldi_dpi0_hrz_ctrl2,
		DSS_WIDTH(rect.w), 13, 0);

	if (is_dual_mipi_panel(hisifd)) {
		dirty_region_updt->ldi_dpi1_hrz_ctrl0 = set_bits32(dirty_region_updt->ldi_dpi1_hrz_ctrl0,
			(h_back_porch + h_porch_pading) << 16, 29, 0);
		dirty_region_updt->ldi_dpi1_hrz_ctrl1 = set_bits32(dirty_region_updt->ldi_dpi1_hrz_ctrl1,
			DSS_WIDTH(pinfo->ldi.h_pulse_width), 13, 0);
		dirty_region_updt->ldi_dpi1_hrz_ctrl2 = set_bits32(dirty_region_updt->ldi_dpi1_hrz_ctrl2,
			DSS_WIDTH(rect.w), 13, 0);
	}

	dirty_region_updt->ldi_vrt_ctrl0 = set_bits32(dirty_region_updt->ldi_vrt_ctrl0,
		(pinfo->ldi.v_front_porch + v_porch_pading) | ((pinfo->ldi.v_back_porch) << 16), 29, 0);
	dirty_region_updt->ldi_vrt_ctrl1 = set_bits32(dirty_region_updt->ldi_vrt_ctrl1,
		DSS_HEIGHT(pinfo->ldi.v_pulse_width), 13, 0);
	dirty_region_updt->ldi_vrt_ctrl2 = set_bits32(dirty_region_updt->ldi_vrt_ctrl2,
		DSS_HEIGHT(rect.h), 13, 0);

	// set dsi size
	dirty_region_updt->edpi_cmd_size = set_bits32(dirty_region_updt->edpi_cmd_size,
		rect.w, 16, 0);

	// set ifbc size
	if (pinfo->ifbc_type != IFBC_TYPE_NONE) {
		if ((pinfo->ifbc_type == IFBC_TYPE_VESA2X_DUAL) ||
			(pinfo->ifbc_type == IFBC_TYPE_VESA3X_DUAL)) {
			dirty_region_updt->ifbc_size = set_bits32(dirty_region_updt->ifbc_size,
				((DSS_WIDTH(dirty.w / 2) << 16) | DSS_HEIGHT(dirty.h)), 32, 0);
		} else {
			dirty_region_updt->ifbc_size = set_bits32(dirty_region_updt->ifbc_size,
				((DSS_WIDTH(dirty.w) << 16) | DSS_HEIGHT(dirty.h)), 32, 0);
		}
	}


	if (pdata && pdata->set_display_region) {
		pdata->set_display_region(hisifd->pdev, &dirty);
	}

	hisifd->dirty_region_updt = dirty;

	hisi_dss_dirty_region_updt_set_reg(hisifd, hisifd->dss_base, &(hisifd->dss_module.dirty_region_updt));

	HISI_FB_DEBUG("dirty_region(%d,%d, %d,%d), h_porch_pading=%d, v_porch_pading=%d.\n",
		dirty.x, dirty.y, dirty.w, dirty.h, h_porch_pading, v_porch_pading);

}


/*******************************************************************************
** WCHN
*/
static uint32_t hisi_calculate_display_addr_wb(bool mmu_enable, dss_wb_layer_t *wb_layer,
	dss_rect_t aligned_rect, dss_rect_t *ov_block_rect, int add_type)
{
	uint32_t addr = 0;
	uint32_t dst_addr = 0;
	uint32_t stride = 0;
	uint32_t offset = 0;
	int left = 0, top = 0;
	int bpp = 0;

	if (wb_layer->transform & HISI_FB_TRANSFORM_ROT_90) {
		left = wb_layer->dst_rect.x;
		top = ov_block_rect->x - wb_layer->dst_rect.x + wb_layer->dst_rect.y;
	} else {
		left = aligned_rect.x;
		top = aligned_rect.y;
	}

	if (add_type == DSS_ADDR_PLANE0) {
		stride = wb_layer->dst.stride;
		dst_addr = mmu_enable ? wb_layer->dst.vir_addr : wb_layer->dst.phy_addr;
	} else if (add_type == DSS_ADDR_PLANE1) {
		stride = wb_layer->dst.stride_plane1;
		offset = wb_layer->dst.offset_plane1;
		dst_addr = mmu_enable ? (wb_layer->dst.vir_addr + offset) : (wb_layer->dst.phy_addr + offset);
		top /= 2;
	} else {
		HISI_FB_ERR("NOT SUPPORT this add_type(%d).\n", add_type);
	}

	bpp = wb_layer->dst.bpp;
	addr = dst_addr + left * bpp + top * stride;

	if (g_debug_ovl_offline_composer) {
		HISI_FB_INFO("addr=0x%x,dst_addr=0x%x,left=%d,top=%d,stride=%d,bpp=%d\n",
			addr, dst_addr, left, top, wb_layer->dst.stride, bpp);
	}
	return addr;
}

int hisi_dss_wdfc_config(struct hisi_fb_data_type *hisifd, dss_wb_layer_t *layer,
	dss_rect_t *aligned_rect, dss_rect_t *ov_block_rect)
{
	dss_dfc_t *dfc = NULL;
	int chn_idx = 0;
	dss_rect_t in_rect;
	bool need_dither = false;

	int size_hrz = 0;
	int size_vrt = 0;
	int dfc_fmt = 0;
	int dfc_pix_in_num = 0;
	int aligned_line = 0;
	uint32_t dfc_w = 0;
	int aligned_pixel = 0;
	int dfc_aligned = 0;

	uint32_t left_pad = 0;
	uint32_t right_pad = 0;
	uint32_t top_pad = 0;
	uint32_t bottom_pad = 0;

	uint32_t addr = 0;
	uint32_t dst_addr = 0;
	uint32_t bpp = 0;
	bool mmu_enable = false;

	if (NULL == hisifd) {
		HISI_FB_ERR("hisifd is NULL");
		return -EINVAL;
	}
	if (NULL == layer) {
		HISI_FB_ERR("layer is NULL");
		return -EINVAL;
	}
	if (NULL == aligned_rect) {
		HISI_FB_ERR("aligned_rect is NULL");
		return -EINVAL;
	}

	chn_idx = layer->chn_idx;

	dfc = &(hisifd->dss_module.dfc[chn_idx]);
	hisifd->dss_module.dfc_used[chn_idx] = 1;

	dfc_fmt = hisi_pixel_format_hal2dfc(layer->dst.format);
	if (dfc_fmt < 0) {
		HISI_FB_ERR("layer format (%d) not support !\n", layer->dst.format);
		return -EINVAL;
	}

	if (layer->need_cap & CAP_AFBCE) {
		aligned_pixel = AFBC_BLOCK_ALIGN;
	} else {
		aligned_pixel = DMA_ALIGN_BYTES / layer->dst.bpp;
	}

	need_dither = isNeedDither(dfc_fmt);
	if (ov_block_rect) {
		memcpy(&in_rect, ov_block_rect, sizeof(dss_rect_t));
	} else {
		in_rect = layer->src_rect;
	}

	size_hrz = DSS_WIDTH(in_rect.w);
	size_vrt = DSS_HEIGHT(in_rect.h);

	if ((size_hrz + 1) % 2 == 1) {
		size_hrz += 1;
		dfc_w = 1;
	}

	dfc_aligned = 0x2;
	if (layer->dst.bpp <= 2) {
		dfc_pix_in_num = 0x1;
		dfc_aligned = 0x4;
	}

	/*lint -e834 -e737 -e502*/
	if (layer->need_cap & CAP_AFBCE) {
		aligned_rect->x = ALIGN_DOWN(in_rect.x, aligned_pixel);
		aligned_rect->w = ALIGN_UP(in_rect.x - aligned_rect->x + in_rect.w + dfc_w, aligned_pixel);
		aligned_rect->y = ALIGN_DOWN(in_rect.y, aligned_pixel);
		aligned_rect->h = ALIGN_UP(in_rect.y - aligned_rect->y + in_rect.h, aligned_pixel);

		left_pad = in_rect.x - aligned_rect->x;
		right_pad = aligned_rect->w - (in_rect.x - aligned_rect->x + in_rect.w + dfc_w);
		top_pad = in_rect.y - aligned_rect->y;
		bottom_pad = aligned_rect->h - (in_rect.y - aligned_rect->y + in_rect.h);
	} else if (layer->need_cap & CAP_HFBCE) {

		left_pad = in_rect.x - aligned_rect->x;
		right_pad = aligned_rect->w - (in_rect.x - aligned_rect->x + in_rect.w + dfc_w);
		top_pad = in_rect.y - aligned_rect->y;
		bottom_pad = aligned_rect->h - (in_rect.y - aligned_rect->y + in_rect.h);
	} else if (layer->transform & HISI_FB_TRANSFORM_ROT_90) {
		aligned_line = (layer->dst.bpp <= 2) ? 32 : 16;  //sp:32bytes ; others:64bytes
		mmu_enable = (layer->dst.mmu_enable == 1) ? true : false;
		dst_addr = mmu_enable ? layer->dst.vir_addr : layer->dst.phy_addr;
		bpp = layer->dst.bpp;
		addr = dst_addr + layer->dst_rect.x * bpp +  (in_rect.x - layer->dst_rect.x + layer->dst_rect.y) * layer->dst.stride;

		if (is_YUV_SP_420(layer->dst.format)) {
			top_pad = (addr & 0x1F) / bpp;
		} else {
			top_pad = (addr & 0x3F) / bpp;
		}

		aligned_rect->x = in_rect.x;
		aligned_rect->y = in_rect.y;
		aligned_rect->w = ALIGN_UP(size_hrz + 1, dfc_aligned);
		aligned_rect->h = ALIGN_UP(in_rect.h + top_pad, aligned_line);

		left_pad = 0;
		right_pad = aligned_rect->w - size_hrz - 1;
		bottom_pad = aligned_rect->h - in_rect.h - top_pad;
	} else {
		aligned_rect->x = ALIGN_DOWN(in_rect.x, aligned_pixel);
		aligned_rect->w = ALIGN_UP(in_rect.x - aligned_rect->x + in_rect.w + dfc_w, aligned_pixel);
		aligned_rect->y = in_rect.y;

		if (is_YUV_SP_420(layer->dst.format)) {
			aligned_rect->h = ALIGN_UP(in_rect.h, 2);
		} else {
			aligned_rect->h = in_rect.h;
		}

		left_pad = in_rect.x - aligned_rect->x;
		right_pad = aligned_rect->w - (left_pad + in_rect.w + dfc_w);
		top_pad = 0;
		bottom_pad = aligned_rect->h - in_rect.h;
	}
	/*lint +e834 +e737 +e502*/
	dfc->disp_size = set_bits32(dfc->disp_size, (size_vrt | (size_hrz << 16)), 32, 0);
	dfc->pix_in_num = set_bits32(dfc->pix_in_num, dfc_pix_in_num, 1, 0);
	dfc->disp_fmt = set_bits32(dfc->disp_fmt, dfc_fmt, 5, 1);
	dfc->clip_ctl_hrz = set_bits32(dfc->clip_ctl_hrz, 0x0, 12, 0);
	dfc->clip_ctl_vrz = set_bits32(dfc->clip_ctl_vrz, 0x0, 12, 0);
	dfc->ctl_clip_en = set_bits32(dfc->ctl_clip_en, 0x0, 1, 0);
	dfc->icg_module = set_bits32(dfc->icg_module, 0x1, 1, 0);
	if (need_dither) {
		dfc->dither_enable = set_bits32(dfc->dither_enable, 0x1, 1, 0);
		dfc->bitext_ctl = set_bits32(dfc->bitext_ctl, 0x3, 32, 0);
	} else {
		dfc->dither_enable = set_bits32(dfc->dither_enable, 0x0, 1, 0);
		dfc->bitext_ctl = set_bits32(dfc->bitext_ctl, 0x0, 32, 0);
	}

	if (left_pad || right_pad || top_pad || bottom_pad) {
		dfc->padding_ctl = set_bits32(dfc->padding_ctl, (left_pad |
			(right_pad << 8) | (top_pad << 16) | (bottom_pad << 24) | (0x1 << 31)), 32, 0);
	} else {
		dfc->padding_ctl = set_bits32(dfc->padding_ctl, 0x0, 32, 0);
	}

	if (g_debug_ovl_offline_composer) {
		HISI_FB_INFO("in_rect[x_y_w_h][%d:%d:%d:%d],align_rect[x_y_w_h][%d:%d:%d:%d]"
			"pad[l_r_t_b][%d:%d:%d:%d],bpp=%d\n", in_rect.x, in_rect.y, in_rect.w, in_rect.h,
			aligned_rect->x, aligned_rect->y, aligned_rect->w, aligned_rect->h,
			left_pad, right_pad, top_pad, bottom_pad, layer->dst.bpp);
	}
	return 0;
}

void hisi_dss_wdma_init(char __iomem *wdma_base, dss_wdma_t *s_wdma)
{
	if (NULL == wdma_base) {
		HISI_FB_ERR("wdma_base is NULL");
		return;
	}
	if (NULL == s_wdma) {
		HISI_FB_ERR("s_wdma is NULL");
		return;
	}

	memset(s_wdma, 0, sizeof(dss_wdma_t));

	s_wdma->oft_x0 = inp32(wdma_base + DMA_OFT_X0);
	s_wdma->oft_y0 = inp32(wdma_base + DMA_OFT_Y0);
	s_wdma->oft_x1 = inp32(wdma_base + DMA_OFT_X1);
	s_wdma->oft_y1 = inp32(wdma_base + DMA_OFT_Y1);
	s_wdma->mask0 = inp32(wdma_base + DMA_MASK0);
	s_wdma->mask1 = inp32(wdma_base + DMA_MASK1);
	s_wdma->stretch_size_vrt = inp32(wdma_base + DMA_STRETCH_SIZE_VRT);
	s_wdma->ctrl = inp32(wdma_base + DMA_CTRL);
	s_wdma->tile_scram = inp32(wdma_base + DMA_TILE_SCRAM);
	s_wdma->sw_mask_en = inp32(wdma_base + WDMA_DMA_SW_MASK_EN);
	s_wdma->start_mask0 = inp32(wdma_base + WDMA_DMA_START_MASK0);
	s_wdma->end_mask0 = inp32(wdma_base + WDMA_DMA_END_MASK1);
	s_wdma->start_mask1 = inp32(wdma_base + WDMA_DMA_START_MASK1);
	s_wdma->end_mask1 = inp32(wdma_base + WDMA_DMA_END_MASK1);
	s_wdma->data_addr = inp32(wdma_base + DMA_DATA_ADDR0);
	s_wdma->stride0 = inp32(wdma_base + DMA_STRIDE0);
	s_wdma->data1_addr = inp32(wdma_base + DMA_DATA_ADDR1);
	s_wdma->stride1 = inp32(wdma_base + DMA_STRIDE1);
	s_wdma->stretch_stride = inp32(wdma_base + DMA_STRETCH_STRIDE0);
	s_wdma->data_num = inp32(wdma_base + DMA_DATA_NUM0);

	s_wdma->ch_rd_shadow = inp32(wdma_base + CH_RD_SHADOW);
	s_wdma->ch_ctl = inp32(wdma_base + CH_CTL);
	s_wdma->ch_secu_en = inp32(wdma_base + CH_SECU_EN);
	s_wdma->ch_sw_end_req = inp32(wdma_base + CH_SW_END_REQ);

	s_wdma->afbce_hreg_pic_blks = inp32(wdma_base + AFBCE_HREG_PIC_BLKS);
	s_wdma->afbce_hreg_format = inp32(wdma_base + AFBCE_HREG_FORMAT);
	s_wdma->afbce_hreg_hdr_ptr_l0 = inp32(wdma_base + AFBCE_HREG_HDR_PTR_L0);
	s_wdma->afbce_hreg_pld_ptr_l0 = inp32(wdma_base + AFBCE_HREG_PLD_PTR_L0);
	s_wdma->afbce_picture_size = inp32(wdma_base + AFBCE_PICTURE_SIZE);
	s_wdma->afbce_ctl = inp32(wdma_base + AFBCE_CTL);
	s_wdma->afbce_header_srtide = inp32(wdma_base + AFBCE_HEADER_SRTIDE);
	s_wdma->afbce_payload_stride = inp32(wdma_base + AFBCE_PAYLOAD_STRIDE);
	s_wdma->afbce_enc_os_cfg = inp32(wdma_base + AFBCE_ENC_OS_CFG);
	s_wdma->afbce_mem_ctrl = inp32(wdma_base + AFBCE_MEM_CTRL);
}

static void hisi_dss_wdma_set_reg(struct hisi_fb_data_type *hisifd,
	char __iomem *wdma_base, dss_wdma_t *s_wdma)
{
	if (hisifd == NULL) {
		HISI_FB_DEBUG("hisifd is NULL!\n");
		return;
	}

	if (wdma_base == NULL) {
		HISI_FB_DEBUG("wdma_base is NULL!\n");
		return;
	}

	if (s_wdma == NULL) {
		HISI_FB_DEBUG("s_wdma is NULL!\n");
		return;
	}

	hisifd->set_reg(hisifd, wdma_base + CH_REG_DEFAULT, 0x1, 32, 0);
	hisifd->set_reg(hisifd, wdma_base + CH_REG_DEFAULT, 0x0, 32, 0);

	hisifd->set_reg(hisifd, wdma_base + DMA_OFT_X0, s_wdma->oft_x0, 32, 0);
	hisifd->set_reg(hisifd, wdma_base + DMA_OFT_Y0, s_wdma->oft_y0, 32, 0);
	hisifd->set_reg(hisifd, wdma_base + DMA_OFT_X1, s_wdma->oft_x1, 32, 0);
	hisifd->set_reg(hisifd, wdma_base + DMA_OFT_Y1, s_wdma->oft_y1, 32, 0);
	hisifd->set_reg(hisifd, wdma_base + DMA_CTRL, s_wdma->ctrl, 32, 0);
	hisifd->set_reg(hisifd, wdma_base + DMA_TILE_SCRAM, s_wdma->tile_scram, 32, 0);
	hisifd->set_reg(hisifd, wdma_base + WDMA_DMA_SW_MASK_EN, s_wdma->sw_mask_en, 32, 0);
	hisifd->set_reg(hisifd, wdma_base + WDMA_DMA_START_MASK0, s_wdma->start_mask0, 32, 0);
	hisifd->set_reg(hisifd, wdma_base + WDMA_DMA_END_MASK0, s_wdma->end_mask0, 32, 0);
	hisifd->set_reg(hisifd, wdma_base + DMA_DATA_ADDR0, s_wdma->data_addr, 32, 0);
	hisifd->set_reg(hisifd, wdma_base + DMA_STRIDE0, s_wdma->stride0, 32, 0);
	hisifd->set_reg(hisifd, wdma_base + DMA_DATA_ADDR1, s_wdma->data1_addr, 32, 0);
	hisifd->set_reg(hisifd, wdma_base + DMA_STRIDE1, s_wdma->stride1, 32, 0);

	//hisifd->set_reg(hisifd, wdma_base + DMA_DATA_NUM0, s_wdma->data_num, 32, 0);

	hisifd->set_reg(hisifd, wdma_base + CH_CTL, s_wdma->ch_ctl, 32, 0);
	hisifd->set_reg(hisifd, wdma_base + ROT_SIZE, s_wdma->rot_size, 32, 0);

	//hisifd->set_reg(hisifd, wdma_base + DMA_BUF_CTRL, s_wdma->dma_buf_ctrl, 32, 0);
	hisifd->set_reg(hisifd, wdma_base + DMA_BUF_SIZE, s_wdma->dma_buf_size, 32, 0);

	if (s_wdma->afbc_used == 1) {
		hisifd->set_reg(hisifd, wdma_base + AFBCE_HREG_PIC_BLKS, s_wdma->afbce_hreg_pic_blks, 32, 0);
		hisifd->set_reg(hisifd, wdma_base + AFBCE_HREG_FORMAT, s_wdma->afbce_hreg_format, 32, 0);
		hisifd->set_reg(hisifd, wdma_base + AFBCE_HREG_HDR_PTR_L0, s_wdma->afbce_hreg_hdr_ptr_l0, 32, 0);
		hisifd->set_reg(hisifd, wdma_base + AFBCE_HREG_PLD_PTR_L0, s_wdma->afbce_hreg_pld_ptr_l0, 32, 0);
		hisifd->set_reg(hisifd, wdma_base + AFBCE_PICTURE_SIZE, s_wdma->afbce_picture_size, 32, 0);
		//hisifd->set_reg(hisifd, wdma_base + AFBCE_CTL, s_wdma->afbce_ctl, 32, 0);
		hisifd->set_reg(hisifd, wdma_base + AFBCE_HEADER_SRTIDE, s_wdma->afbce_header_srtide, 32, 0);
		hisifd->set_reg(hisifd, wdma_base + AFBCE_PAYLOAD_STRIDE, s_wdma->afbce_payload_stride, 32, 0);
		hisifd->set_reg(hisifd, wdma_base + AFBCE_ENC_OS_CFG, s_wdma->afbce_enc_os_cfg, 32, 0);
		//hisifd->set_reg(hisifd, wdma_base + AFBCE_MEM_CTRL, s_wdma->afbce_mem_ctrl, 32, 0);
		//hisifd->set_reg(hisifd, wdma_base + AFBCE_QOS_CFG, s_wdma->afbce_qos_cfg, 32, 0);
		hisifd->set_reg(hisifd, wdma_base + AFBCE_THRESHOLD, s_wdma->afbce_threshold, 32, 0);
		hisifd->set_reg(hisifd, wdma_base + AFBCE_SCRAMBLE_MODE, s_wdma->afbce_scramble_mode, 32, 0);
		hisifd->set_reg(hisifd, wdma_base + AFBCE_HEADER_POINTER_OFFSET, s_wdma->afbce_header_pointer_offset, 32, 0);
	}

}

static int hisi_dss_wdma_afbc_check_header (dss_wb_layer_t *layer,dss_rect_t in_rect)
{
	if(NULL == layer) {
		HISI_FB_ERR("layer is NULL");
		return -EINVAL;
	}

	if ((layer->dst.width & (AFBC_HEADER_ADDR_ALIGN - 1)) ||
		(layer->dst.height & (AFBC_BLOCK_ALIGN - 1))) {
		HISI_FB_ERR("wb_layer img width(%d) is not %d bytes aligned, or "
			"img heigh(%d) is not %d bytes aligned!\n",
			layer->dst.width, AFBC_HEADER_ADDR_ALIGN,
			layer->dst.height, AFBC_BLOCK_ALIGN);
		return -EINVAL;
	}

	if ((in_rect.w < AFBC_PIC_WIDTH_MIN) || (in_rect.w > AFBCE_IN_WIDTH_MAX) ||
		(in_rect.h < AFBC_PIC_HEIGHT_MIN) || (in_rect.h > AFBC_PIC_HEIGHT_MAX) ||
		(in_rect.w & (AFBC_BLOCK_ALIGN - 1)) || (in_rect.h & (AFBC_BLOCK_ALIGN - 1))) {
		HISI_FB_ERR("afbce in_rect(%d,%d, %d,%d) is out of range!",
			in_rect.x, in_rect.y, in_rect.w, in_rect.h);
		return -EINVAL;
	}
	return  0;
}

static int hisi_dss_wdma_afbc_check_payload(dss_wb_layer_t *layer,dss_rect_ltrb_t afbc_payload_rect,
	dss_rect_t in_rect,uint32_t *afbc_payload_addr,uint32_t *afbc_payload_stride)
{
	uint32_t stride_align = 0;
	uint32_t addr_align = 0;

	if(NULL == layer) {
		HISI_FB_ERR("layer is NULL");
		return -EINVAL;
	}
	if(NULL == afbc_payload_addr || NULL == afbc_payload_stride){
		HISI_FB_ERR("stride and addr is NULL");
		return -EINVAL;
	}

	if (layer->dst.bpp == 4) {
		stride_align = AFBC_PAYLOAD_STRIDE_ALIGN_32;
		addr_align = AFBC_PAYLOAD_ADDR_ALIGN_32;
	} else if (layer->dst.bpp == 2) {
		stride_align = AFBC_PAYLOAD_STRIDE_ALIGN_16;
		addr_align = AFBC_PAYLOAD_ADDR_ALIGN_16;
	} else {
		HISI_FB_ERR("bpp(%d) not supported!\n", layer->dst.bpp);
		return -EINVAL;
	}

	*afbc_payload_stride = layer->dst.afbc_payload_stride;
	if (layer->dst.afbc_scramble_mode != DSS_AFBC_SCRAMBLE_MODE2) {
		*afbc_payload_stride = (layer->dst.width / AFBC_BLOCK_ALIGN) * stride_align;
	}
	*afbc_payload_addr = layer->dst.afbc_payload_addr +
		(afbc_payload_rect.top / AFBC_BLOCK_ALIGN) * (*afbc_payload_stride) +
		(afbc_payload_rect.left / AFBC_BLOCK_ALIGN) * stride_align;

	if ((*afbc_payload_addr & (addr_align - 1)) ||
		(*afbc_payload_stride & (stride_align - 1))) {
		HISI_FB_ERR("afbc_payload_addr(0x%x) is not %d bytes aligned, or "
			"afbc_payload_stride(0x%x) is not %d bytes aligned!\n",
			*afbc_payload_addr, addr_align,
			*afbc_payload_stride, stride_align);
			return -EINVAL;
	}

	if (g_debug_ovl_online_composer || g_debug_ovl_offline_composer) {
	HISI_FB_INFO("aligned_rect(%d,%d,%d,%d), afbc_rect(%d,%d,%d,%d)!\n",
		in_rect.x, in_rect.y,
		DSS_WIDTH(in_rect.x + in_rect.w), DSS_WIDTH(in_rect.y + in_rect.h),
		afbc_payload_rect.left, afbc_payload_rect.top, afbc_payload_rect.right, afbc_payload_rect.bottom);
		}

	return 0 ;
}

int hisi_dss_wdma_config(struct hisi_fb_data_type *hisifd, dss_overlay_t *pov_req,
	dss_wb_layer_t *layer, dss_rect_t aligned_rect, dss_rect_t *ov_block_rect, bool last_block)
{
	dss_wdma_t *wdma = NULL;
	int chn_idx = 0;
	int wdma_format = 0;
	int wdma_transform = 0;
	uint32_t oft_x0 = 0;
	uint32_t oft_x1 = 0;
	uint32_t oft_y0 = 0;
	uint32_t oft_y1 = 0;
	uint32_t data_num = 0;
	uint32_t wdma_addr = 0;
	uint32_t wdma_stride = 0;

	uint32_t wdma_buf_width = 0;
	uint32_t wdma_buf_height = 0;

	dss_rect_t in_rect;
	int temp = 0;
	int aligned_pixel = 0;
	int l2t_interleave_n = 0;
	bool mmu_enable = false;

	dss_rect_ltrb_t afbc_header_rect = {0};
	dss_rect_ltrb_t afbc_payload_rect = {0};
	uint32_t afbce_hreg_pic_blks;
	uint32_t afbc_header_addr = 0;
	uint32_t afbc_header_stride = 0;
	uint32_t afbc_payload_addr = 0;
	uint32_t afbc_payload_stride = 0;
	int32_t afbc_header_start_pos = 0;
	uint32_t afbc_header_pointer_offset = 0;
	int ret =0;
	if (NULL == hisifd || NULL == pov_req || NULL == layer) {
		HISI_FB_ERR("NULL ptr.\n");
		return -EINVAL;
	}

	if (NULL == ov_block_rect) {
		HISI_FB_ERR("NULL ptr.\n");
		return -EINVAL;
	}

	chn_idx = layer->chn_idx;

	wdma = &(hisifd->dss_module.wdma[chn_idx]);
	hisifd->dss_module.dma_used[chn_idx] = 1;

	wdma_format = hisi_pixel_format_hal2dma(layer->dst.format);
	if (wdma_format < 0) {
		HISI_FB_ERR("hisi_pixel_format_hal2dma failed!\n");
		return -EINVAL;
	}

	in_rect = aligned_rect;
	aligned_pixel = DMA_ALIGN_BYTES / layer->dst.bpp;

	wdma_transform = hisi_transform_hal2dma(layer->transform, chn_idx);
	if (wdma_transform < 0) {
		HISI_FB_ERR("hisi_transform_hal2dma failed!\n");
		return -EINVAL;
	}

	mmu_enable = (layer->dst.mmu_enable == 1) ? true : false;
	wdma_addr = mmu_enable ? layer->dst.vir_addr : layer->dst.phy_addr;

	if (layer->need_cap & CAP_HFBCE) {
		return 0;
	}

	if (layer->need_cap & CAP_AFBCE) {
		wdma->afbc_used = 1;

		ret = hisi_dss_wdma_afbc_check_header(layer,in_rect);
		if (ret != 0) {
			HISI_FB_ERR("hisi_dss_afbc header error! ret = %d\n", ret);
			return -EINVAL;
		}
		afbc_header_rect.right = ALIGN_UP(in_rect.x + in_rect.w, AFBC_HEADER_ADDR_ALIGN) - 1;
		afbc_header_rect.bottom = ALIGN_UP(in_rect.y + in_rect.h, AFBC_BLOCK_ALIGN) - 1;
		if (layer->transform & HISI_FB_TRANSFORM_ROT_90) {
			afbc_header_rect.left = ALIGN_DOWN(layer->dst_rect.x, AFBC_HEADER_ADDR_ALIGN);
			afbc_header_rect.top = ALIGN_DOWN(layer->dst_rect.y + (ov_block_rect->x - layer->dst_rect.x),
				AFBC_BLOCK_ALIGN);

			afbc_payload_rect.left = ALIGN_DOWN(layer->dst_rect.x, AFBC_BLOCK_ALIGN);
			afbc_payload_rect.top = afbc_header_rect.top;

			afbc_header_start_pos = (layer->dst_rect.x - afbc_header_rect.left) / AFBC_BLOCK_ALIGN;
		} else {
			afbc_header_rect.left = ALIGN_DOWN(in_rect.x, AFBC_HEADER_ADDR_ALIGN);
			afbc_header_rect.top = ALIGN_DOWN(in_rect.y, AFBC_BLOCK_ALIGN);

			afbc_payload_rect.left = ALIGN_DOWN(in_rect.x, AFBC_BLOCK_ALIGN);
			afbc_payload_rect.top = afbc_header_rect.top;

			afbc_header_start_pos = (in_rect.x - afbc_header_rect.left) / AFBC_BLOCK_ALIGN;
		}

		if (afbc_header_start_pos < 0) {
			HISI_FB_ERR("afbc_header_start_pos(%d) is invalid!\n", afbc_header_start_pos);
			return -EINVAL;
		}

		afbce_hreg_pic_blks = (in_rect.w / AFBC_BLOCK_ALIGN) * (in_rect.h / AFBC_BLOCK_ALIGN) - 1;

		// afbc header
		afbc_header_stride = (layer->dst.width / AFBC_BLOCK_ALIGN) * AFBC_HEADER_STRIDE_BLOCK;
		afbc_header_pointer_offset = (afbc_header_rect.top / AFBC_BLOCK_ALIGN) * afbc_header_stride +
			(afbc_header_rect.left / AFBC_BLOCK_ALIGN) * AFBC_HEADER_STRIDE_BLOCK;
		afbc_header_addr = layer->dst.afbc_header_addr + afbc_header_pointer_offset;

		if ((afbc_header_addr & (AFBC_HEADER_ADDR_ALIGN - 1)) ||
			(afbc_header_stride & (AFBC_HEADER_STRIDE_ALIGN - 1))) {
			HISI_FB_ERR("wb_layer afbc_header_addr(0x%x) is not %d bytes aligned, or "
				"afbc_header_stride(0x%x) is not %d bytes aligned!\n",
				afbc_header_addr, AFBC_HEADER_ADDR_ALIGN,
				afbc_header_stride, AFBC_HEADER_STRIDE_ALIGN);
			return -EINVAL;
		}

		//afbc payload
		ret = hisi_dss_wdma_afbc_check_payload(layer,afbc_payload_rect,in_rect,&afbc_payload_addr,&afbc_payload_stride);
		if(ret != 0) {
			HISI_FB_ERR("hisi_dss_afbc payload error! ret = %d\n", ret);
			return -EINVAL;
		}

		wdma->ctrl = set_bits32(wdma->ctrl, wdma_format, 5, 3);
		wdma->ctrl = set_bits32(wdma->ctrl, (mmu_enable ? 0x1 : 0x0), 1, 8);
		wdma->ctrl = set_bits32(wdma->ctrl, wdma_transform, 3, 9);
		if (last_block) {
			wdma->ch_ctl = set_bits32(wdma->ch_ctl, 0x1d, 5, 0);
		} else {
			wdma->ch_ctl = set_bits32(wdma->ch_ctl, 0xd, 5, 0);
		}

		wdma->rot_size = set_bits32(wdma->rot_size,
			(DSS_WIDTH(in_rect.w) | (DSS_HEIGHT(in_rect.h) << 16)), 32, 0);

		wdma->afbce_hreg_pic_blks = set_bits32(wdma->afbce_hreg_pic_blks, afbce_hreg_pic_blks, 24, 0);
		//color transform
		wdma->afbce_hreg_format = set_bits32(wdma->afbce_hreg_format,
			(isYUVPackage(layer->dst.format) ? 0x0 : 0x1), 1, 21);
		wdma->afbce_hreg_hdr_ptr_l0 = set_bits32(wdma->afbce_hreg_hdr_ptr_l0, afbc_header_addr, 32, 0);
		wdma->afbce_hreg_pld_ptr_l0 = set_bits32(wdma->afbce_hreg_pld_ptr_l0, afbc_payload_addr, 32, 0);
		wdma->afbce_picture_size = set_bits32(wdma->afbce_picture_size,
			((DSS_WIDTH(in_rect.w) << 16) | DSS_HEIGHT(in_rect.h)), 32, 0);
		wdma->afbce_header_srtide = set_bits32(wdma->afbce_header_srtide,
			((afbc_header_start_pos << 14) | afbc_header_stride), 16, 0);
		wdma->afbce_payload_stride = set_bits32(wdma->afbce_payload_stride, afbc_payload_stride, 20, 0);
		wdma->afbce_enc_os_cfg = set_bits32(wdma->afbce_enc_os_cfg, DSS_AFBCE_ENC_OS_CFG_DEFAULT_VAL, 3, 0);
		wdma->afbce_mem_ctrl = set_bits32(wdma->afbce_mem_ctrl, 0x0, 12, 0);
		wdma->afbce_threshold = set_bits32(wdma->afbce_threshold, 0x2, 32, 0);
		wdma->afbce_header_pointer_offset = set_bits32(wdma->afbce_header_pointer_offset,
			afbc_header_pointer_offset, 32, 0);
		//afbcd_scramble_mode
		wdma->afbce_scramble_mode = set_bits32(wdma->afbce_scramble_mode,
			layer->dst.afbc_scramble_mode, 2, 0);

		return 0;
	}

	if (layer->need_cap & CAP_TILE) {
		l2t_interleave_n = hisi_get_rdma_tile_interleave(layer->dst.stride);
		if (l2t_interleave_n < MIN_INTERLEAVE) {
			HISI_FB_ERR("tile stride should be 256*2^n, error stride:%d!\n", layer->dst.stride);
			return -EINVAL;
		}

		if (wdma_addr & (TILE_DMA_ADDR_ALIGN - 1)) {
			HISI_FB_ERR("tile wdma_addr(0x%x) is not %d bytes aligned.\n",
				wdma_addr, TILE_DMA_ADDR_ALIGN);
			return -EINVAL;
		}
	}

	if (layer->transform & HISI_FB_TRANSFORM_ROT_90) {
		temp = in_rect.w;
		in_rect.w = in_rect.h;
		in_rect.h = temp;

		oft_x0 = 0;
		oft_x1 = DSS_WIDTH(in_rect.w) / aligned_pixel;
		oft_y0 = 0;
		oft_y1 = DSS_HEIGHT(ov_block_rect->w);
	} else {
		oft_x0 = in_rect.x / aligned_pixel;
		oft_x1 = DSS_WIDTH(in_rect.x + in_rect.w) / aligned_pixel;
		oft_y0 = in_rect.y;
		oft_y1 = DSS_HEIGHT(in_rect.y + in_rect.h);
	}

	wdma_addr = hisi_calculate_display_addr_wb(mmu_enable, layer, in_rect, ov_block_rect, DSS_ADDR_PLANE0);
	wdma_stride = layer->dst.stride / DMA_ALIGN_BYTES;

	data_num = (oft_x1 - oft_x0 + 1) * (oft_y1- oft_y0 + 1);

	if (layer->transform & HISI_FB_TRANSFORM_ROT_90) {
		wdma->rot_size = set_bits32(wdma->rot_size,
			(DSS_WIDTH(ov_block_rect->w) | (DSS_HEIGHT(aligned_rect.h) << 16)), 32, 0);

		if (ov_block_rect) {
			wdma_buf_width = DSS_HEIGHT(ov_block_rect->h);
			wdma_buf_height = DSS_WIDTH(ov_block_rect->w);
		} else {
			wdma_buf_width = DSS_HEIGHT(layer->src_rect.h);
			wdma_buf_height = DSS_WIDTH(layer->src_rect.w);
		}
	} else {
		if (ov_block_rect) {
			wdma_buf_width = DSS_WIDTH(ov_block_rect->w);
			wdma_buf_height = DSS_HEIGHT(ov_block_rect->h);
		} else {
			wdma_buf_width = DSS_WIDTH(layer->src_rect.w);
			wdma_buf_height = DSS_HEIGHT(layer->src_rect.h);
		}
	}

	wdma->oft_x0 = set_bits32(wdma->oft_x0, oft_x0, 12, 0);
	wdma->oft_y0 = set_bits32(wdma->oft_y0, oft_y0, 16, 0);
	wdma->oft_x1 = set_bits32(wdma->oft_x1, oft_x1, 12, 0);
	wdma->oft_y1 = set_bits32(wdma->oft_y1, oft_y1, 16, 0);

	wdma->ctrl = set_bits32(wdma->ctrl, wdma_format, 5, 3);
	wdma->ctrl = set_bits32(wdma->ctrl, wdma_transform, 3, 9);
	wdma->ctrl = set_bits32(wdma->ctrl, (mmu_enable ? 0x1 : 0x0), 1, 8);
	wdma->data_num = set_bits32(wdma->data_num, data_num, 30, 0);

	wdma->ctrl = set_bits32(wdma->ctrl, ((layer->need_cap & CAP_TILE) ? 0x1 : 0x0), 1, 1);
	wdma->tile_scram = set_bits32(wdma->tile_scram, ((layer->need_cap & CAP_TILE) ? 0x1 : 0x0), 1, 0);

	wdma->data_addr = set_bits32(wdma->data_addr, wdma_addr, 32, 0);
	wdma->stride0 = set_bits32(wdma->stride0, wdma_stride | (l2t_interleave_n << 16) , 20, 0);

	if (is_YUV_SP_420(layer->dst.format)) {
		wdma_addr = hisi_calculate_display_addr_wb(mmu_enable, layer, in_rect, ov_block_rect, DSS_ADDR_PLANE1);
		wdma_stride = layer->dst.stride_plane1 / DMA_ALIGN_BYTES;
		wdma->data1_addr = set_bits32(wdma->data1_addr, wdma_addr, 32, 0);
		wdma->stride1 = set_bits32(wdma->stride1, wdma_stride, 13, 0);
	}

	if (last_block) {
		wdma->ch_ctl = set_bits32(wdma->ch_ctl, 1, 1, 4);
	} else {
		wdma->ch_ctl = set_bits32(wdma->ch_ctl, 0, 1, 4);
	}
	wdma->ch_ctl = set_bits32(wdma->ch_ctl, 1, 1, 3);
	wdma->ch_ctl = set_bits32(wdma->ch_ctl, 1, 1, 0);

	wdma->dma_buf_size = set_bits32(wdma->dma_buf_size,
		wdma_buf_width | (wdma_buf_height << 16), 32, 0);

	if (g_debug_ovl_offline_composer) {
		HISI_FB_INFO("dma_oft[x0_y0_x1_y1][%d:%d:%d:%d],rot_size[w_h][%d:%d],"
			"wdma_buf_size[w_h][%d:%d]\n", oft_x0, oft_y0, oft_x1, oft_y1,
			ov_block_rect->w, aligned_rect.h, wdma_buf_width, wdma_buf_height);
	}

	return 0;
}


/*******************************************************************************
** DSS GLOBAL
*/
int hisi_dss_module_init(struct hisi_fb_data_type *hisifd)
{
	if (NULL == hisifd) {
		HISI_FB_ERR("hisifd is NULL");
		return -EINVAL;
	}

	if (hisifd->index == MEDIACOMMON_PANEL_IDX) {
		memcpy(&(hisifd->dss_module), &(hisifd->dss_mdc_module_default), sizeof(dss_module_reg_t));
	} else {
		memcpy(&(hisifd->dss_module), &(hisifd->dss_module_default), sizeof(dss_module_reg_t));
	}

	return 0;
}

/*lint -e661 -e662*/
int hisi_dss_module_default(struct hisi_fb_data_type *hisifd)
{
	dss_module_reg_t *dss_module = NULL;
	uint32_t module_base = 0;
	char __iomem *dss_base = NULL;
	int i = 0;

	if (NULL == hisifd) {
		HISI_FB_ERR("hisifd is NULL");
		return -EINVAL;
	}
	dss_base = hisifd->dss_base;
	if (NULL == dss_base) {
		HISI_FB_ERR("dss_base is NULL");
		return -EINVAL;
	}

	dss_module = &(hisifd->dss_module_default);
	memset(dss_module, 0, sizeof(dss_module_reg_t));

	for (i = 0; i < DSS_MCTL_IDX_MAX; i++) {
		module_base = g_dss_module_ovl_base[i][MODULE_MCTL_BASE];
		if (module_base != 0) {
			dss_module->mctl_base[i] = dss_base + module_base;
			hisi_dss_mctl_init(dss_module->mctl_base[i], &(dss_module->mctl[i]));
		}
	}

	for (i = 0; i < DSS_OVL_IDX_MAX; i++) {
		module_base = g_dss_module_ovl_base[i][MODULE_OVL_BASE];
		if (module_base != 0) {
			dss_module->ov_base[i] = dss_base + module_base;
			hisi_dss_ovl_init(dss_module->ov_base[i], &(dss_module->ov[i]), i);
		}
	}

	for (i = 0; i < DSS_CHN_MAX_DEFINE; i++) {


		module_base = g_dss_module_base[i][MODULE_AIF0_CHN];
		if (module_base != 0) {
			dss_module->aif_ch_base[i] = dss_base + module_base;
			hisi_dss_aif_init(dss_module->aif_ch_base[i], &(dss_module->aif[i]));
		}

		module_base = g_dss_module_base[i][MODULE_AIF1_CHN];
		if (module_base != 0) {
			dss_module->aif1_ch_base[i] = dss_base + module_base;
			hisi_dss_aif_init(dss_module->aif1_ch_base[i], &(dss_module->aif1[i]));
		}

		module_base = g_dss_module_base[i][MODULE_MIF_CHN];
		if (module_base != 0) {
			dss_module->mif_ch_base[i] = dss_base + module_base;
			hisi_dss_mif_init(dss_module->mif_ch_base[i], &(dss_module->mif[i]), i);
		}

		module_base = g_dss_module_base[i][MODULE_MCTL_CHN_MUTEX];
		if (module_base != 0) {
			dss_module->mctl_ch_base[i].chn_mutex_base = dss_base + module_base;
		}

		module_base = g_dss_module_base[i][MODULE_MCTL_CHN_FLUSH_EN];
		if (module_base != 0) {
			dss_module->mctl_ch_base[i].chn_flush_en_base = dss_base + module_base;
		}

		module_base = g_dss_module_base[i][MODULE_MCTL_CHN_OV_OEN];

		if (module_base != 0) {
			dss_module->mctl_ch_base[i].chn_ov_en_base = dss_base + module_base;
		}

		module_base = g_dss_module_base[i][MODULE_MCTL_CHN_STARTY];
		if (module_base != 0) {
			dss_module->mctl_ch_base[i].chn_starty_base = dss_base + module_base;
			hisi_dss_mctl_ch_starty_init(dss_module->mctl_ch_base[i].chn_starty_base,
				&(dss_module->mctl_ch[i]));
		}

		module_base = g_dss_module_base[i][MODULE_MCTL_CHN_MOD_DBG];
		if (module_base != 0) {
			dss_module->mctl_ch_base[i].chn_mod_dbg_base = dss_base + module_base;
			hisi_dss_mctl_ch_mod_dbg_init(dss_module->mctl_ch_base[i].chn_mod_dbg_base,
				&(dss_module->mctl_ch[i]));
		}

		module_base = g_dss_module_base[i][MODULE_DMA];
		if (module_base != 0) {
			dss_module->dma_base[i] = dss_base + module_base;
			if (i < DSS_WCHN_W0 || i == DSS_RCHN_V2) {
				hisi_dss_rdma_init(dss_module->dma_base[i], &(dss_module->rdma[i]));
			} else {
				hisi_dss_wdma_init(dss_module->dma_base[i], &(dss_module->wdma[i]));
			}

			if ((i == DSS_RCHN_V0) || (i == DSS_RCHN_V1) || (i == DSS_RCHN_V2)) {
				hisi_dss_rdma_u_init(dss_module->dma_base[i], &(dss_module->rdma[i]));
				hisi_dss_rdma_v_init(dss_module->dma_base[i], &(dss_module->rdma[i]));
			}
		}

		module_base = g_dss_module_base[i][MODULE_DFC];
		if (module_base != 0) {
			dss_module->dfc_base[i] = dss_base + module_base;
			hisi_dss_dfc_init(dss_module->dfc_base[i], &(dss_module->dfc[i]));
		}

		module_base = g_dss_module_base[i][MODULE_SCL];
		if (module_base != 0) {
			dss_module->scl_base[i] = dss_base + module_base;
			hisi_dss_scl_init(dss_module->scl_base[i], &(dss_module->scl[i]));
		}


		module_base = DSS_POST_SCF_OFFSET;

		if (module_base != 0) {
			dss_module->post_scf_base = dss_base + module_base;
			hisi_dss_post_scf_init(dss_base, dss_module->post_scf_base, &(dss_module->post_scf));
		}

		module_base = g_dss_module_base[i][MODULE_PCSC];
		if (module_base != 0) {
			dss_module->pcsc_base[i] = dss_base + module_base;
			hisi_dss_csc_init(dss_module->pcsc_base[i], &(dss_module->pcsc[i]));
		}

		module_base = g_dss_module_base[i][MODULE_ARSR2P];
		if (module_base != 0) {
			dss_module->arsr2p_base[i] = dss_base + module_base;
			hisi_dss_arsr2p_init(dss_module->arsr2p_base[i], &(dss_module->arsr2p[i]));
		}

		module_base = g_dss_module_base[i][MODULE_POST_CLIP];
		if (module_base != 0){
			dss_module->post_clip_base[i] = dss_base + module_base;
			hisi_dss_post_clip_init(dss_module->post_clip_base[i], &(dss_module->post_clip[i]));
		}

		module_base = g_dss_module_base[i][MODULE_CSC];
		if (module_base != 0) {
			dss_module->csc_base[i] = dss_base + module_base;
			hisi_dss_csc_init(dss_module->csc_base[i], &(dss_module->csc[i]));
		}
	}

	module_base = DSS_MCTRL_SYS_OFFSET;
	if (module_base != 0) {
		dss_module->mctl_sys_base = dss_base + module_base;
		hisi_dss_mctl_sys_init(dss_module->mctl_sys_base, &(dss_module->mctl_sys));
	}

	module_base = DSS_SMMU_OFFSET;
	if (module_base != 0) {
		dss_module->smmu_base = dss_base + module_base;
		hisi_dss_smmu_init(dss_module->smmu_base, &(dss_module->smmu));
	}

	return 0;
}
/*lint +e661 +e662*/
int hisi_dss_ch_module_set_regs(struct hisi_fb_data_type *hisifd, int32_t mctl_idx, int chn_idx, uint32_t wb_type, bool enable_cmdlist)
{
	dss_module_reg_t *dss_module = NULL;
	int i = 0;
	int ret = 0;
	uint32_t tmp = 0;

	if (hisifd == NULL) {
		HISI_FB_DEBUG("hisifd is NULL!\n");
		return -1;
	}

	if ((chn_idx < 0) || (chn_idx >= DSS_CHN_MAX_DEFINE)) {
		HISI_FB_DEBUG("chn_idx is out of the range!\n");
		return -1;
	}

	dss_module = &(hisifd->dss_module);
	i = chn_idx;

	if (enable_cmdlist) {
		if (chn_idx == DSS_RCHN_V2) {  //chicago copybit
			tmp = (0x1 << DSS_CMDLIST_V2);
			hisifd->cmdlist_idx = DSS_CMDLIST_V2;
		} else if (chn_idx == DSS_WCHN_W2) {
			tmp = (0x1 << DSS_CMDLIST_W2);
			hisifd->cmdlist_idx = DSS_CMDLIST_W2;
		} else {
			tmp = (0x1 << chn_idx);
			hisifd->cmdlist_idx = chn_idx;
		}

		//FIXME:base, dim
		//add rch cmdlist
		ret = hisi_cmdlist_add_new_node(hisifd, tmp, 0, 0, 0, 0, wb_type);
		if (ret != 0) {
			HISI_FB_ERR("fb%d, hisi_cmdlist_add_new_node err:%d \n", hisifd->index, ret);
			goto err_return;
		}
	}

	if (dss_module->mctl_ch_used[i] == 1) {
		hisi_dss_mctl_ch_set_reg(hisifd,
			&(dss_module->mctl_ch_base[i]), &(dss_module->mctl_ch[i]), mctl_idx, i);
	}

	if (dss_module->smmu_used == 1) {
		hisi_dss_smmu_ch_set_reg(hisifd, dss_module->smmu_base, &(dss_module->smmu), i);
	}

	if (dss_module->dma_used[i] == 1) {
		if (i < DSS_WCHN_W0 || i == DSS_RCHN_V2) {
			hisi_dss_rdma_set_reg(hisifd, dss_module->dma_base[i], &(dss_module->rdma[i]));
		} else {
			hisi_dss_wdma_set_reg(hisifd, dss_module->dma_base[i], &(dss_module->wdma[i]));
		}

		if ((i == DSS_RCHN_V0) || (i == DSS_RCHN_V1) || (i == DSS_RCHN_V2)) {
			hisi_dss_rdma_u_set_reg(hisifd, dss_module->dma_base[i], &(dss_module->rdma[i]));
			hisi_dss_rdma_v_set_reg(hisifd, dss_module->dma_base[i], &(dss_module->rdma[i]));
		}
	}

	if (dss_module->aif_ch_used[i] == 1) {
		hisi_dss_aif_ch_set_reg(hisifd, dss_module->aif_ch_base[i], &(dss_module->aif[i]));
	}

	if (dss_module->aif1_ch_used[i] == 1) {
		hisi_dss_aif_ch_set_reg(hisifd, dss_module->aif1_ch_base[i], &(dss_module->aif1[i]));
	}

	if (dss_module->mif_used[i] == 1) {
		hisi_dss_mif_set_reg(hisifd, dss_module->mif_ch_base[i], &(dss_module->mif[i]), i);
	}

	if (dss_module->dfc_used[i] == 1) {
		hisi_dss_dfc_set_reg(hisifd, dss_module->dfc_base[i], &(dss_module->dfc[i]));
	}

	if (dss_module->scl_used[i] == 1) {
		hisi_dss_chn_scl_load_filter_coef_set_reg(hisifd, false, chn_idx, dss_module->scl[i].fmt);
		hisi_dss_scl_set_reg(hisifd, dss_module->scl_base[i], &(dss_module->scl[i]));
	}


	if (hisifd->dss_module.post_clip_used[i]) {
		hisi_dss_post_clip_set_reg( hisifd, dss_module->post_clip_base[i], &(dss_module->post_clip[i]), i);
	}

	if (dss_module->pcsc_used[i] == 1) {
		hisi_dss_csc_set_reg(hisifd, dss_module->pcsc_base[i], &(dss_module->pcsc[i]));
	}

	if (dss_module->arsr2p_used[i] == 1) {
		hisi_dss_arsr2p_set_reg(hisifd, dss_module->arsr2p_base[i], &(dss_module->arsr2p[i]));
	}

	if (dss_module->csc_used[i] == 1) {
		hisi_dss_csc_set_reg(hisifd, dss_module->csc_base[i], &(dss_module->csc[i]));
	}

	if (dss_module->mctl_ch_used[i] == 1) {
		hisi_dss_mctl_sys_ch_set_reg(hisifd,
			&(dss_module->mctl_ch_base[i]), &(dss_module->mctl_ch[i]), i, true);
	}

	return 0;

err_return:
	return ret;
}

int hisi_dss_ov_module_set_regs(struct hisi_fb_data_type *hisifd, dss_overlay_t *pov_req, int ovl_idx,
	bool enable_cmdlist, int task_end, int last, bool is_first_ov_block)
{
	dss_module_reg_t *dss_module = NULL;
	int i = 0;
	int ret = 0;
	uint32_t tmp = 0;

	if (hisifd == NULL) {
		HISI_FB_DEBUG("hisifd is NULL!\n");
		return -1;
	}


	if (pov_req && pov_req->wb_layer_infos[0].chn_idx == DSS_WCHN_W2) { //chicago copybit no ovl
		return 0;
	}

	dss_module = &(hisifd->dss_module);
	i = ovl_idx;

	if (enable_cmdlist) {
		//add ov cmdlist
		tmp = (0x1 << (DSS_CMDLIST_OV0 + ovl_idx));
		hisifd->cmdlist_idx = DSS_CMDLIST_OV0 + ovl_idx;

		ret = hisi_cmdlist_add_new_node(hisifd, tmp, 0, task_end, 0, last, 0);
		if (ret != 0) {
			HISI_FB_ERR("fb%d, hisi_cmdlist_add_new_node err:%d \n", hisifd->index, ret);
			goto err_return;
		}
	}

	if (dss_module->mctl_used[i] == 1) {
		hisi_dss_mctl_ov_set_reg(hisifd, dss_module->mctl_base[i],	&(dss_module->mctl[i]), ovl_idx, enable_cmdlist);
	}

	if (is_first_ov_block) {
		if (dss_module->dirty_region_updt_used == 1)
			hisi_dss_dirty_region_dbuf_set_reg(hisifd, hisifd->dss_base, &(dss_module->dirty_region_updt));
	}

	if (dss_module->ov_used[i] == 1) {
		hisi_dss_ovl_set_reg(hisifd, dss_module->ov_base[i], &(dss_module->ov[i]), ovl_idx);
	}

	if (hisifd->dss_module.post_scf_used == 1) {
		hisi_dss_post_scf_set_reg(hisifd, hisifd->dss_module.post_scf_base, &(hisifd->dss_module.post_scf));
	}

	if (is_first_ov_block) {
		hisi_dss_dpp_acm_gm_set_reg(hisifd);
		hisi_dss_dpp_ace_set_reg(hisifd);
		hisi_dss_dpp_acm_gmp_set_reg(hisifd);
		hisi_dss_dpp_hiace_set_reg(hisifd);
		hisi_dss_effect_set_reg(hisifd);
	}

	if (dss_module->mctl_sys_used == 1) {
		hisi_dss_mctl_sys_set_reg(hisifd, dss_module->mctl_sys_base, &(dss_module->mctl_sys), ovl_idx);
	}

	return 0;

err_return:
	return ret;
}

static void get_use_comm_mmbuf(int *use_comm_mmbuf,
	dss_mmbuf_t *offline_mmbuf, struct dss_comm_mmbuf_info *online_mmbuf)
{
	int i = 0;
	int j = 0;

	for (i = 0; i < DSS_CHN_MAX_DEFINE; i++) {
		for (j = 0; j < DSS_CHN_MAX_DEFINE; j++) {
			if ((((offline_mmbuf[i].addr < online_mmbuf[j].mmbuf.addr + online_mmbuf[j].mmbuf.size) &&
				(offline_mmbuf[i].addr >= online_mmbuf[j].mmbuf.addr))
				|| ((online_mmbuf[j].mmbuf.addr < offline_mmbuf[i].addr + offline_mmbuf[i].size) &&
				(online_mmbuf[j].mmbuf.addr >= offline_mmbuf[i].addr)))
				&& offline_mmbuf[i].size) {
				if (use_comm_mmbuf) {
					*use_comm_mmbuf |= 1 << online_mmbuf[j].ov_idx;
					online_mmbuf[j].mmbuf.addr = 0;
					online_mmbuf[j].mmbuf.size = 0;
					break;
				}
			}
		}
	}

	if (g_debug_dump_mmbuf) {
		for (i = 0; i < DSS_CHN_MAX_DEFINE; i++) {
			HISI_FB_INFO("g_online_mmbuf[%d].addr=0x%x, size=%d!\n",
			i, online_mmbuf[i].mmbuf.addr, online_mmbuf[i].mmbuf.size);
		}
	}
}

/*lint -e574 -e737*/
static void hisi_dss_check_use_comm_mmbuf(uint32_t display_id,
	int *use_comm_mmbuf, dss_mmbuf_t *offline_mmbuf, bool has_rot)
{
	int i = 0;

	if (display_id == PRIMARY_PANEL_IDX) {
		get_use_comm_mmbuf(use_comm_mmbuf, offline_mmbuf, g_primary_online_mmbuf);
		get_use_comm_mmbuf(use_comm_mmbuf, offline_mmbuf, g_external_online_mmbuf);

		if (!has_rot) {
			memset(g_primary_online_mmbuf, 0x0, sizeof(g_primary_online_mmbuf));
		}
	}

	if (display_id == EXTERNAL_PANEL_IDX) {
		get_use_comm_mmbuf(use_comm_mmbuf, offline_mmbuf, g_external_online_mmbuf);
		get_use_comm_mmbuf(use_comm_mmbuf, offline_mmbuf, g_primary_online_mmbuf);

		if (!has_rot) {
			memset(g_external_online_mmbuf, 0x0, sizeof(g_external_online_mmbuf));
		}
	}

	if (g_debug_dump_mmbuf) {
		for (i = 0; i < DSS_CHN_MAX_DEFINE; i++) {
			HISI_FB_INFO("offline_mmbuf[%d].addr=0x%x, size=%d, *use_comm_mmbuf=%d!\n",
			i, offline_mmbuf[i].addr, offline_mmbuf[i].size, *use_comm_mmbuf);
		}
	}
}
/*lint +e574 +e737*/

/*lint -e438 -e550*/
int hisi_dss_prev_module_set_regs(struct hisi_fb_data_type *hisifd,
	dss_overlay_t *pov_req, uint32_t cmdlist_pre_idxs, bool enable_cmdlist, int *use_comm_mmbuf)
{
	dss_module_reg_t *dss_module = NULL;
	dss_layer_t *layer = NULL;
	dss_wb_layer_t *wb_layer = NULL;
	int32_t ovl_idx = 0;
	int32_t layer_idx = 0;
	int32_t mctl_idx = 0;
	bool has_ovl = true;
	int chn_idx = 0;
	int i = 0;
	int j = 0;
	int k = 0;
	int m = 0;
	bool has_base = false;
	int ret = 0;
	uint32_t tmp = 0;
	uint32_t cmdlist_idxs_temp = 0;
	dss_overlay_block_t *pov_h_block_infos = NULL;
	dss_overlay_block_t *pov_h_block = NULL;
	dss_mmbuf_t offline_mmbuf[DSS_CHN_MAX_DEFINE];
	bool has_rot = false;
	uint32_t display_id = 0;

	if (NULL == hisifd) {
		HISI_FB_ERR("hisifd is NULL");
		return -EINVAL;
	}
	if (NULL == pov_req) {
		HISI_FB_ERR("pov_req is NULL");
		return -EINVAL;
	}

	ovl_idx = pov_req->ovl_idx;
	dss_module = &(hisifd->dss_module);

	if (enable_cmdlist) {
		//clear prev chn cmdlist reg default
		if (pov_req->wb_enable) {
			ret = hisi_cmdlist_add_new_node(hisifd, cmdlist_pre_idxs, 0, 1, 1, 1, 0);
		} else {
			ret = hisi_cmdlist_add_new_node(hisifd, cmdlist_pre_idxs, 0, 0, 0, 0, 0);
		}
		if(ret != 0) {
			HISI_FB_ERR("fb%d, hisi_cmdlist_add_new_node err:%d \n", hisifd->index, ret);
			goto err_return;
		}
	}

	if (hisifd->index == MEDIACOMMON_PANEL_IDX) {
		has_ovl = false;
	}

	memset(offline_mmbuf, 0x0, sizeof(offline_mmbuf));
	cmdlist_idxs_temp = cmdlist_pre_idxs;
	pov_h_block_infos = (dss_overlay_block_t *)pov_req->ov_block_infos_ptr;
	for (m = 0; m < pov_req->ov_block_nums; m++) {
		pov_h_block = &(pov_h_block_infos[m]);

		for (i = 0; i < pov_h_block->layer_nums; i++) {
			layer = &(pov_h_block->layer_infos[i]);
			chn_idx = layer->chn_idx;
			layer_idx = layer->layer_idx;

			if ((chn_idx == DSS_RCHN_V2) && (g_dss_version_tag == FB_ACCEL_HI366x)) {  //chicago copybit
				mctl_idx = DSS_MCTL5;
				has_ovl = false;
			} else {
				mctl_idx = ovl_idx;
			}

			if (layer->need_cap & (CAP_BASE | CAP_DIM | CAP_PURE_COLOR)) {
				if (layer->need_cap & CAP_BASE )
					has_base = true;

				continue;
			}

			if ((layer->need_cap & (CAP_AFBCD | CAP_HFBCD)) && (layer->dst_rect.y >= pov_h_block->ov_block_rect.y)) {
				if (chn_idx < DSS_CHN_MAX_DEFINE) {
					if (ovl_idx == DSS_OVL0) {
						g_primary_online_mmbuf[chn_idx].mmbuf.addr = layer->img.mmbuf_base;
						g_primary_online_mmbuf[chn_idx].mmbuf.size = layer->img.mmbuf_size;
						g_primary_online_mmbuf[chn_idx].ov_idx = ovl_idx;
					}

					if (ovl_idx == DSS_OVL1) {
						g_external_online_mmbuf[chn_idx].mmbuf.addr = layer->img.mmbuf_base;
						g_external_online_mmbuf[chn_idx].mmbuf.size = layer->img.mmbuf_size;
						g_external_online_mmbuf[chn_idx].ov_idx = ovl_idx;
					}
				}
			}

			if ((ovl_idx == DSS_OVL2) || (ovl_idx == DSS_OVL3)) {
				if (layer->need_cap & (CAP_AFBCD | CAP_HFBCD)) {
					if (j < DSS_CHN_MAX_DEFINE) {
						offline_mmbuf[j].addr = layer->img.mmbuf_base;
						offline_mmbuf[j].size = layer->img.mmbuf_size;
						j++;
					}
				}
			}

			if (chn_idx == DSS_RCHN_V2) {
				tmp = (0x1 << DSS_CMDLIST_V2);
				hisifd->cmdlist_idx = DSS_CMDLIST_V2;
			} else {
				tmp = (0x1 << chn_idx);
				hisifd->cmdlist_idx = chn_idx;
			}

			if ((cmdlist_idxs_temp & tmp) != tmp) {
				continue;
			} else {
				cmdlist_idxs_temp &= (~tmp);
			}

			// RCH default
			hisi_dss_chn_set_reg_default_value(hisifd, dss_module->dma_base[chn_idx]);

			// SMMU
			hisi_dss_smmu_ch_set_reg(hisifd, dss_module->smmu_base, &(dss_module->smmu), chn_idx);

			// MIF
			hisi_dss_mif_set_reg(hisifd, dss_module->mif_ch_base[chn_idx], &(dss_module->mif[chn_idx]), chn_idx);

			// AIF
			hisi_dss_aif_ch_set_reg(hisifd, dss_module->aif_ch_base[chn_idx], &(dss_module->aif[chn_idx]));

			/* MCTL */
			dss_module->mctl_ch[chn_idx].chn_mutex =
				set_bits32(dss_module->mctl_ch[chn_idx].chn_mutex, 1, 1, 0);
			dss_module->mctl_ch[chn_idx].chn_flush_en =
				set_bits32(dss_module->mctl_ch[chn_idx].chn_flush_en, 1, 1, 0);
			dss_module->mctl_ch[chn_idx].chn_ov_oen =
				set_bits32(dss_module->mctl_ch[chn_idx].chn_ov_oen, 0, 32, 0);
			dss_module->mctl_ch_used[chn_idx] = 1;

			hisi_dss_mctl_sys_ch_set_reg(hisifd, &(dss_module->mctl_ch_base[chn_idx]),
				&(dss_module->mctl_ch[chn_idx]), chn_idx, false);
		}
	}

	if (pov_req->wb_enable && ((ovl_idx > DSS_OVL1) || (!has_ovl))) {
		if (has_ovl) {
			hisifd->cmdlist_idx = DSS_CMDLIST_OV0 + ovl_idx;

			// OV default
			hisi_dss_ov_set_reg_default_value(hisifd, dss_module->ov_base[ovl_idx], ovl_idx);
		}

		for (k = 0; k < pov_req->wb_layer_nums; k++) {
			wb_layer = &(pov_req->wb_layer_infos[k]);
			chn_idx = wb_layer->chn_idx;
			display_id = wb_layer->dst.display_id;
			if (wb_layer->transform & HISI_FB_TRANSFORM_ROT_90) {
				has_rot = true;
			}

			if (chn_idx == DSS_WCHN_W2) {  //chicago copybit
				hisifd->cmdlist_idx = DSS_CMDLIST_W2;
			} else {
				hisifd->cmdlist_idx = chn_idx;
			}

			// WCH default
			hisi_dss_chn_set_reg_default_value(hisifd, dss_module->dma_base[chn_idx]);
			// MIF
			hisi_dss_mif_set_reg(hisifd, dss_module->mif_ch_base[chn_idx], &(dss_module->mif[chn_idx]), chn_idx);
			// AIF
			hisi_dss_aif_ch_set_reg(hisifd, dss_module->aif_ch_base[chn_idx], &(dss_module->aif[chn_idx]));

			// MCTL ch
			dss_module->mctl_ch[chn_idx].chn_mutex =
				set_bits32(dss_module->mctl_ch[chn_idx].chn_mutex, 0x1, 1, 0);
			dss_module->mctl_ch[chn_idx].chn_flush_en =
				set_bits32(dss_module->mctl_ch[chn_idx].chn_flush_en, 0x1, 1, 0);
			dss_module->mctl_ch[chn_idx].chn_ov_oen =
				set_bits32(dss_module->mctl_ch[chn_idx].chn_ov_oen, 0x0, 32, 0);
			dss_module->mctl_ch_used[chn_idx] = 1;

			hisi_dss_mctl_sys_ch_set_reg(hisifd, &(dss_module->mctl_ch_base[chn_idx]),
				&(dss_module->mctl_ch[chn_idx]), chn_idx, false);
		}

		if (has_ovl) {    //chicago copybit
			hisifd->cmdlist_idx = DSS_CMDLIST_OV0 + ovl_idx;

			// MCTL ov
			dss_module->mctl_sys.chn_ov_sel_used[ovl_idx] = 1;
			dss_module->mctl_sys.wch_ov_sel_used[ovl_idx - DSS_OVL2] = 1;
			dss_module->mctl_sys.ov_flush_en_used[ovl_idx] = 1;
			dss_module->mctl_sys.ov_flush_en[ovl_idx] = set_bits32(dss_module->mctl_sys.ov_flush_en[ovl_idx], 0x1, 1, 0);
			hisi_dss_mctl_sys_set_reg(hisifd, dss_module->mctl_sys_base, &(dss_module->mctl_sys), ovl_idx);
		}

		//remove mctl ch & ov

		hisi_dss_check_use_comm_mmbuf(display_id, use_comm_mmbuf, offline_mmbuf, has_rot);
	}

	return 0;

err_return:
	return ret;
}
/*lint +e438 +e550*/

void hisi_dss_unflow_handler(struct hisi_fb_data_type *hisifd,
	dss_overlay_t *pov_req, bool unmask)
{
	uint32_t tmp = 0;
	char __iomem *ldi_base = NULL;
	if (NULL == hisifd) {
		HISI_FB_ERR("hisifd is NULL");
		return;
	}

	ldi_base = hisifd->dss_base + DSS_LDI1_OFFSET;

	if (pov_req->ovl_idx == DSS_OVL0) {
		tmp = inp32(hisifd->dss_base + DSS_LDI0_OFFSET + LDI_CPU_ITF_INT_MSK);
		if (unmask) {
			tmp &= ~ BIT_LDI_UNFLOW;
		} else {
			tmp |= BIT_LDI_UNFLOW;
		}
		outp32(hisifd->dss_base + DSS_LDI0_OFFSET + LDI_CPU_ITF_INT_MSK, tmp);

	} else if (pov_req->ovl_idx == DSS_OVL1) {
		tmp = inp32(ldi_base + LDI_CPU_ITF_INT_MSK);
		if (unmask) {
			tmp &= ~ BIT_LDI_UNFLOW;
		} else {
			tmp |= BIT_LDI_UNFLOW;
		}
		outp32(ldi_base + LDI_CPU_ITF_INT_MSK, tmp);
	} else {
		; /* do nothing */
	}
}


/*******************************************************************************
** compose handler
*/
int hisi_ov_compose_handler(struct hisi_fb_data_type *hisifd,
	dss_overlay_t *pov_req,
	dss_overlay_block_t *pov_h_block,
	dss_layer_t *layer,
	dss_rect_t *wb_dst_rect,
	dss_rect_t *wb_ov_block_rect,
	dss_rect_ltrb_t *clip_rect,
	dss_rect_t *aligned_rect,
	bool *rdma_stretch_enable,
	bool *has_base,
	bool csc_needed,
	bool enable_cmdlist)
{
	int ret = 0;
	int32_t mctl_idx = 0;

	if (NULL == hisifd) {
		HISI_FB_ERR("hisifd is NULL");
		return -EINVAL;
	}
	if (NULL == pov_h_block) {
		HISI_FB_ERR("pov_h_block is NULL");
		return -EINVAL;
	}
	if (NULL == layer) {
		HISI_FB_ERR("layer is NULL");
		return -EINVAL;
	}
	if (NULL == clip_rect) {
		HISI_FB_ERR("clip_rect is NULL");
		return -EINVAL;
	}
	if (NULL == aligned_rect) {
		HISI_FB_ERR("aligned_rect is NULL");
		return -EINVAL;
	}
	if (NULL == rdma_stretch_enable) {
		HISI_FB_ERR("rdma_stretch_enable is NULL");
		return -EINVAL;
	}

	ret = hisi_dss_check_layer_par(hisifd, layer);
	if (ret != 0) {
		HISI_FB_ERR("hisi_dss_check_layer_par failed! ret = %d\n", ret);
		goto err_return;
	}

	if (layer->need_cap & (CAP_BASE | CAP_DIM | CAP_PURE_COLOR)) {
		if (layer->need_cap & CAP_BASE)
			*has_base = true;

		ret = hisi_dss_ovl_layer_config(hisifd, pov_req, layer, wb_ov_block_rect, *has_base);
		if (ret != 0) {
			HISI_FB_ERR("hisi_dss_ovl_config failed! need_cap=0x%x, ret=%d\n",
				layer->need_cap, ret);
			goto err_return;
		}

		ret = hisi_dss_mctl_ch_config(hisifd, pov_req, layer, NULL, pov_req->ovl_idx, wb_ov_block_rect, *has_base);
		if (ret != 0) {
			HISI_FB_ERR("hisi_dss_mctl_ch_config failed! ret = %d\n", ret);
			goto err_return;
		}

		return ret;
	}

	if (g_debug_ovl_block_composer) {
		HISI_FB_INFO("layer->dst_rect.y=%d, pov_h_block->ov_block_rect.y=%d,"
			"layer->chn_idx=%d, layer->layer_idx=%d\n",
			layer->dst_rect.y, pov_h_block->ov_block_rect.y, layer->chn_idx, layer->layer_idx);
	}

	if (layer->dst_rect.y < pov_h_block->ov_block_rect.y) {
		if (g_debug_ovl_block_composer) {
			HISI_FB_INFO("layer->dst_rect.y=%d, pov_h_block->ov_block_rect.y=%d,"
				"layer->chn_idx=%d, layer->layer_idx=%d!!!!\n",
				layer->dst_rect.y, pov_h_block->ov_block_rect.y, layer->chn_idx, layer->layer_idx);
		}

		ret = hisi_dss_ovl_layer_config(hisifd, pov_req, layer, wb_ov_block_rect, *has_base);
		if (ret != 0) {
			HISI_FB_ERR("hisi_dss_ovl_config failed, ret = %d\n", ret);
			goto err_return;
		}

		ret = hisi_dss_mctl_ch_config(hisifd, pov_req, layer, NULL, pov_req->ovl_idx, wb_ov_block_rect, *has_base);
		if (ret != 0) {
			HISI_FB_ERR("hisi_dss_mctl_ch_config failed! ret = %d\n", ret);
			goto err_return;
		}

		return ret;
	}

	ret = hisi_dss_smmu_ch_config(hisifd, layer, NULL);
	if (ret != 0) {
		HISI_FB_ERR("hisi_dss_smmu_ch_config failed! ret = %d\n", ret);
		goto err_return;
	}

	if (g_debug_ovl_online_composer || g_debug_ovl_offline_composer) {
		HISI_FB_INFO("fb%d, rdma input, src_rect(%d,%d,%d,%d), src_rect_mask(%d,%d,%d,%d), dst_rect(%d,%d,%d,%d).\n",
			hisifd->index,
			layer->src_rect.x, layer->src_rect.y, layer->src_rect.w, layer->src_rect.h,
			layer->src_rect_mask.x, layer->src_rect_mask.y, layer->src_rect_mask.w, layer->src_rect_mask.h,
			layer->dst_rect.x, layer->dst_rect.y, layer->dst_rect.w, layer->dst_rect.h);
	}

	ret = hisi_dss_rdma_config(hisifd, pov_req->ovl_idx, layer,
		clip_rect, aligned_rect, rdma_stretch_enable);
	if (ret != 0) {
		HISI_FB_ERR("hisi_dss_rdma_config failed! ret = %d\n", ret);
		goto err_return;
	}

	if (g_debug_ovl_online_composer || g_debug_ovl_offline_composer) {
		HISI_FB_INFO("fb%d, rdma output, clip_rect(%d,%d,%d,%d), aligned_rect(%d,%d,%d,%d), dst_rect(%d,%d,%d,%d).\n",
			hisifd->index, clip_rect->left, clip_rect->right, clip_rect->top, clip_rect->bottom,
			aligned_rect->x, aligned_rect->y, aligned_rect->w, aligned_rect->h,
			layer->dst_rect.x, layer->dst_rect.y, layer->dst_rect.w, layer->dst_rect.h);
	}

	ret = hisi_dss_aif_ch_config(hisifd, pov_req, layer, wb_dst_rect, NULL, pov_req->ovl_idx);
	if (ret != 0) {
		HISI_FB_ERR("hisi_dss_aif_ch_config failed! ret = %d\n", ret);
		goto err_return;
	}

	ret = hisi_dss_aif1_ch_config(hisifd, pov_req, layer, NULL, pov_req->ovl_idx);
	if (ret != 0) {
		HISI_FB_ERR("hisi_dss_aif1_ch_config failed! ret = %d\n", ret);
		goto err_return;
	}

	ret = hisi_dss_mif_config(hisifd, layer, NULL, *rdma_stretch_enable);
	if (ret != 0) {
		HISI_FB_ERR("hisi_dss_mif_config failed! ret = %d\n", ret);
		goto err_return;
	}

	ret = hisi_dss_rdfc_config(hisifd, layer, aligned_rect, *clip_rect);
	if (ret != 0) {
		HISI_FB_ERR("hisi_dss_rdfc_config failed! ret = %d\n", ret);
		goto err_return;
	}

	if (g_debug_ovl_online_composer || g_debug_ovl_offline_composer) {
		HISI_FB_INFO("fb%d, rdfc output, clip_rect(%d,%d,%d,%d), aligned_rect(%d,%d,%d,%d), dst_rect(%d,%d,%d,%d).\n",
			hisifd->index, clip_rect->left, clip_rect->right, clip_rect->top, clip_rect->bottom,
			aligned_rect->x, aligned_rect->y, aligned_rect->w, aligned_rect->h,
			layer->dst_rect.x, layer->dst_rect.y, layer->dst_rect.w, layer->dst_rect.h);
	}

	ret = hisi_dss_scl_config(hisifd, layer, aligned_rect, *rdma_stretch_enable);
	if (ret != 0) {
		HISI_FB_ERR("hisi_dss_scl_config failed! ret = %d\n", ret);
		goto err_return;
	}

	ret = hisi_dss_arsr2p_config(hisifd, layer, aligned_rect, *rdma_stretch_enable);
	if (ret != 0) {
		HISI_FB_ERR("hisi_dss_arsr2p_config failed! ret = %d\n", ret);
		goto err_return;
	}

	ret = hisi_dss_post_clip_config(hisifd, layer);
	if (ret != 0) {
		HISI_FB_ERR("hisi_dss_post_clip_config failed! ret = %d\n", ret);
		goto err_return;
	}


	if (g_debug_ovl_online_composer || g_debug_ovl_offline_composer) {
		HISI_FB_INFO("fb%d, scf output, clip_rect(%d,%d,%d,%d), aligned_rect(%d,%d,%d,%d), dst_rect(%d,%d,%d,%d).\n",
			hisifd->index, clip_rect->left, clip_rect->right, clip_rect->top, clip_rect->bottom,
			aligned_rect->x, aligned_rect->y, aligned_rect->w, aligned_rect->h,
			layer->dst_rect.x, layer->dst_rect.y, layer->dst_rect.w, layer->dst_rect.h);
	}

	if (csc_needed) {
		ret = hisi_dss_csc_config(hisifd, layer, NULL);
		if (ret != 0) {
			HISI_FB_ERR("hisi_dss_csc_config failed! ret = %d\n", ret);
			goto err_return;
		}
	}

	ret = hisi_dss_ovl_layer_config(hisifd, pov_req, layer, wb_ov_block_rect, *has_base);
	if (ret != 0) {
		HISI_FB_ERR("hisi_dss_ovl_config failed, ret = %d\n", ret);
		goto err_return;
	}

	ret = hisi_dss_mctl_ch_config(hisifd, pov_req, layer, NULL, pov_req->ovl_idx, wb_ov_block_rect, *has_base);
	if (ret != 0) {
		HISI_FB_ERR("hisi_dss_mctl_ch_config failed! ret = %d\n", ret);
		goto err_return;
	}

	if ((g_dss_version_tag == FB_ACCEL_HI366x) && (layer->chn_idx == DSS_RCHN_V2)) {   //copybit
		mctl_idx = DSS_MCTL5;
	} else {
		mctl_idx = pov_req->ovl_idx;
	}

	ret = hisi_dss_ch_module_set_regs(hisifd, mctl_idx, layer->chn_idx, 0, enable_cmdlist);
	if (ret != 0) {
		HISI_FB_ERR("fb%d, hisi_dss_ch_module_set_regs failed! ret = %d\n", hisifd->index, ret);
		goto err_return;
	}

	return 0;

err_return:
	return ret;
}

int hisi_wb_compose_handler(struct hisi_fb_data_type *hisifd,
	dss_overlay_t *pov_req,
	dss_wb_layer_t *wb_layer,
	dss_rect_t *wb_ov_block_rect,
	bool last_block,
	uint32_t wb_type,
	bool csc_needed,
	bool enable_cmdlist)
{
	int ret = 0;
	int32_t mctl_idx = 0;
	dss_rect_t aligned_rect;

	if (hisifd == NULL) {
		HISI_FB_ERR("hisifd is NULL Point!");
		return -EINVAL;
	}

	if (pov_req == NULL) {
		HISI_FB_ERR("pov_req is NULL Point!");
		return -EINVAL;
	}

	if (csc_needed) {
		ret = hisi_dss_csc_config(hisifd, NULL, wb_layer);
		if (ret != 0) {
			HISI_FB_ERR("hisi_dss_csc_config failed! ret = %d\n", ret);
			goto err_return;
		}
	}

	if (g_debug_ovl_online_composer || g_debug_ovl_offline_composer) {
		HISI_FB_INFO("fb%d, wdfc input, src_rect(%d,%d,%d,%d), dst_rect(%d,%d,%d,%d).\n",
			hisifd->index,
			wb_layer->src_rect.x, wb_layer->src_rect.y, wb_layer->src_rect.w, wb_layer->src_rect.h,
			wb_layer->dst_rect.x, wb_layer->dst_rect.y, wb_layer->dst_rect.w, wb_layer->dst_rect.h);
	}

	if (ret != 0) {
		HISI_FB_ERR("hisi_dss_wb_scl_config failed, ret = %d\n", ret);
		goto err_return;
	}

	ret = hisi_dss_wdfc_config(hisifd, wb_layer, &aligned_rect, wb_ov_block_rect);
	if (ret != 0) {
		HISI_FB_ERR("hisi_dss_wdfc_config failed, ret = %d\n", ret);
		goto err_return;
	}

	if (g_debug_ovl_online_composer || g_debug_ovl_offline_composer) {
		HISI_FB_INFO("fb%d, wdfc output, aligned_rect(%d,%d,%d,%d), dst_rect(%d,%d,%d,%d).\n",
			hisifd->index, aligned_rect.x, aligned_rect.y, aligned_rect.w, aligned_rect.h,
			wb_layer->dst_rect.x, wb_layer->dst_rect.y, wb_layer->dst_rect.w, wb_layer->dst_rect.h);
	}

	ret = hisi_dss_wdma_config(hisifd, pov_req, wb_layer, aligned_rect, wb_ov_block_rect, last_block);
	if (ret != 0) {
		HISI_FB_ERR("hisi_dss_wdma_config failed, ret = %d\n", ret);
		goto err_return;
	}

	if (g_debug_ovl_online_composer || g_debug_ovl_offline_composer) {
		HISI_FB_INFO("fb%d, wdma output, aligned_rect(%d,%d,%d,%d), dst_rect(%d,%d,%d,%d).\n",
			hisifd->index, aligned_rect.x, aligned_rect.y, aligned_rect.w, aligned_rect.h,
			wb_layer->dst_rect.x, wb_layer->dst_rect.y, wb_layer->dst_rect.w, wb_layer->dst_rect.h);
	}

	ret = hisi_dss_mif_config(hisifd, NULL, wb_layer, false);
	if (ret != 0) {
		HISI_FB_ERR("hisi_dss_mif_config failed! ret = %d\n", ret);
		goto err_return;
	}

	ret = hisi_dss_aif1_ch_config(hisifd, pov_req, NULL, wb_layer, pov_req->ovl_idx);
	if (ret != 0) {
		HISI_FB_ERR("hisi_dss_aif1_ch_config failed! ret = %d\n", ret);
		goto err_return;
	}

	ret = hisi_dss_aif_ch_config(hisifd, pov_req, NULL, NULL, wb_layer, pov_req->ovl_idx);
	if (ret != 0) {
		HISI_FB_ERR("hisi_dss_aif_ch_config failed! ret = %d\n", ret);
		goto err_return;
	}

	ret = hisi_dss_smmu_ch_config(hisifd, NULL, wb_layer);
	if (ret != 0) {
		HISI_FB_ERR("hisi_dss_smmu_ch_config failed! ret = %d\n", ret);
		goto err_return;
	}

	ret = hisi_dss_mctl_ch_config(hisifd, pov_req, NULL, wb_layer, pov_req->ovl_idx, wb_ov_block_rect, 0);
	if (ret != 0) {
		HISI_FB_ERR("hisi_dss_mctl_ch_config failed! ret = %d\n", ret);
		goto err_return;
	}

	if (wb_layer->chn_idx == DSS_WCHN_W2) {   //chicago copybit
		mctl_idx = DSS_MCTL5;
	} else {
		mctl_idx = pov_req->ovl_idx;
	}

	ret = hisi_dss_ch_module_set_regs(hisifd, mctl_idx, wb_layer->chn_idx, wb_type, enable_cmdlist);
	if (ret != 0) {
		HISI_FB_ERR("fb%d, hisi_dss_ch_module_set_regs failed! ret = %d\n", hisifd->index, ret);
		goto err_return;
	}

	return 0;

err_return:
	return ret;
}

/*******************************************************************************
**
*/
DEFINE_SEMAPHORE(hisi_dss_mmbuf_sem);
static int mmbuf_refcount = 0;
static int dss_sr_refcount = 0;

struct hisifb_mmbuf {
	struct list_head list_node;
	uint32_t addr;
	uint32_t size;
};

static struct list_head *g_mmbuf_list = NULL;

static void hisifb_dss_on(struct hisi_fb_data_type *hisifd, int enable_cmdlist)
{
	int prev_refcount = 0;

	if (NULL == hisifd) {
		HISI_FB_ERR("hisifd is NULL");
		return;
	}

	HISI_FB_DEBUG("fb%d, +.\n", hisifd->index);

	down(&hisi_dss_mmbuf_sem);

	prev_refcount = dss_sr_refcount++;
	if (!prev_refcount) {
		// dss qos on
		hisi_dss_qos_on(hisifd);
		// mmbuf on
		hisi_dss_mmbuf_on(hisifd);
		// mif on
		hisi_dss_mif_on(hisifd);
		// smmu on
		hisi_dss_smmu_on(hisifd);
		// scl coef load
		hisi_dss_scl_coef_on(hisifd, false, SCL_COEF_YUV_IDX);//lint !e747

		if (enable_cmdlist) {
			hisi_dss_cmdlist_qos_on(hisifd);
		}
	}
	up(&hisi_dss_mmbuf_sem);

	HISI_FB_DEBUG("fb%d, -, dss_sr_refcount=%d.\n", hisifd->index, dss_sr_refcount);
}

static void hisifb_dss_off(struct hisi_fb_data_type *hisifd, bool is_lp)
{
	struct hisifb_mmbuf *node, *_node_;
	int new_refcount = 0;

	if (NULL == hisifd) {
		HISI_FB_ERR("hisifd is NULL");
		return;
	}

	if (hisifd->index == MEDIACOMMON_PANEL_IDX) {
		return;
	}
	HISI_FB_DEBUG("fb%d, +.\n", hisifd->index);

	down(&hisi_dss_mmbuf_sem);
	new_refcount = --dss_sr_refcount;
	if (new_refcount < 0) {
		HISI_FB_ERR("dss new_refcount err");
	}

	if (is_lp) {
		if (!new_refcount) {
			hisifd->ldi_data_gate_en = 0;

			memset(&(hisifd->ov_block_infos_prev_prev), 0,
				HISI_DSS_OV_BLOCK_NUMS * sizeof(dss_overlay_block_t));
			memset(&(hisifd->ov_block_infos_prev), 0,
				HISI_DSS_OV_BLOCK_NUMS * sizeof(dss_overlay_block_t));
			memset(&(hisifd->ov_block_infos), 0,
				HISI_DSS_OV_BLOCK_NUMS * sizeof(dss_overlay_block_t));

			hisifb_dss_overlay_info_init(&hisifd->ov_req);
			hisifb_dss_overlay_info_init(&hisifd->ov_req_prev);
			hisifb_dss_overlay_info_init(&hisifd->ov_req_prev_prev);
		}
	}

	if (!g_mmbuf_list || is_lp) {
		up(&hisi_dss_mmbuf_sem);
		return ;
	}

	if (!new_refcount) {
		list_for_each_entry_safe(node, _node_, g_mmbuf_list, list_node) {
			if ((node->addr > 0) && (node->size > 0)) {
				gen_pool_free(hisifd->mmbuf_gen_pool, node->addr, node->size);
				HISI_FB_DEBUG("hisi_dss_mmbuf_free, addr=0x%x, size=%d.\n", node->addr, node->size);
			}

			list_del(&node->list_node);
			kfree(node);
		}
	}
	up(&hisi_dss_mmbuf_sem);

	HISI_FB_DEBUG("fb%d, -, dss_sr_refcount=%d.\n", hisifd->index, dss_sr_refcount);
}

void* hisi_dss_mmbuf_init(struct hisi_fb_data_type *hisifd)
{
	struct gen_pool *pool = NULL;
	int order = 3;
	size_t size = MMBUF_SIZE_MAX;
	uint32_t addr = MMBUF_BASE;
	int prev_refcount = 0;

	if (NULL == hisifd) {
		HISI_FB_ERR("hisifd is NULL");
		return NULL;
	}

	HISI_FB_DEBUG("fb%d, +.\n", hisifd->index);

	down(&hisi_dss_mmbuf_sem);

	prev_refcount = mmbuf_refcount++;
	if (!prev_refcount) {
		//mmbuf pool
		pool = gen_pool_create(order, 0);
		if (pool == NULL) {
			HISI_FB_ERR("fb%d, gen_pool_create failed!", hisifd->index);
			goto err_out;
		}

		if (gen_pool_add(pool, addr, size, 0) != 0) {
			gen_pool_destroy(pool);
			pool = NULL;
			HISI_FB_ERR("fb%d, gen_pool_add failed!", hisifd->index);
			goto err_out;
		}

		g_mmbuf_gen_pool = pool;

		//mmbuf list
		if (!g_mmbuf_list) {
			g_mmbuf_list = kzalloc(sizeof(struct list_head), GFP_KERNEL);
			if (NULL == g_mmbuf_list) {
				HISI_FB_ERR("g_mmbuf_list is NULL");
				up(&hisi_dss_mmbuf_sem);
				return NULL;
			}
			INIT_LIST_HEAD(g_mmbuf_list);
		}

		//smmu
		if (!g_smmu_rwerraddr_virt) {
			g_smmu_rwerraddr_virt = kmalloc(SMMU_RW_ERR_ADDR_SIZE, GFP_KERNEL|__GFP_DMA);
			if (g_smmu_rwerraddr_virt) {
				memset(g_smmu_rwerraddr_virt, 0, SMMU_RW_ERR_ADDR_SIZE);
			} else {
				HISI_FB_ERR("kmalloc g_smmu_rwerraddr_virt fail.\n");
			}
		}
	}

	hisifd->mmbuf_gen_pool = g_mmbuf_gen_pool;
	hisifd->mmbuf_list = g_mmbuf_list;

err_out:
	up(&hisi_dss_mmbuf_sem);

	HISI_FB_DEBUG("fb%d, -, mmbuf_refcount=%d.\n", hisifd->index, mmbuf_refcount);

	return pool;
}

void hisi_dss_mmbuf_deinit(struct hisi_fb_data_type *hisifd)
{
	int new_refcount = 0;

	if (NULL == hisifd) {
		HISI_FB_ERR("hisifd is NULL");
		return;
	}

	HISI_FB_DEBUG("fb%d, +.\n", hisifd->index);

	hisifb_dss_off(hisifd, false);

	down(&hisi_dss_mmbuf_sem);
	new_refcount = --mmbuf_refcount;
	if (new_refcount < 0) {
		HISI_FB_ERR("dss new_refcount err");
	}

	if (!new_refcount) {
		//mmbuf pool
		if (g_mmbuf_gen_pool) {
			gen_pool_destroy(g_mmbuf_gen_pool);
			g_mmbuf_gen_pool = NULL;
		}

		//mmbuf list
		if (g_mmbuf_list) {
			kfree(g_mmbuf_list);
			g_mmbuf_list = NULL;
		}

		//smmu
		if (g_smmu_rwerraddr_virt) {
			kfree(g_smmu_rwerraddr_virt);
			g_smmu_rwerraddr_virt = NULL;
		}
	}

	hisifd->mmbuf_gen_pool = NULL;
	hisifd->mmbuf_list = NULL;
	up(&hisi_dss_mmbuf_sem);

	HISI_FB_DEBUG("fb%d, -, mmbuf_refcount=%d.\n", hisifd->index, mmbuf_refcount);
}

uint32_t hisi_dss_mmbuf_alloc(void *handle, uint32_t size)
{
	uint32_t addr = 0;
	uint32_t mmbuf_size_max;
	struct hisifb_mmbuf *node = NULL;
	struct hisifb_mmbuf *mmbuf_node, *_node_;

	if (NULL == handle) {
		HISI_FB_ERR("handle is NULL!\n");
		return addr;
	}

	if (NULL == g_mmbuf_list) {
		HISI_FB_ERR("g_mmbuf_list is NULL!\n");
		return addr;
	}

	mmbuf_size_max = MMBUF_SIZE_MAX;
	if (size <= 0 || size > mmbuf_size_max) {
		HISI_FB_ERR("mmbuf size is invalid, size=%d!\n", size);
		return addr;
	}

	down(&hisi_dss_mmbuf_sem);

	addr = gen_pool_alloc(handle, size);

	if (addr <= 0) {
		list_for_each_entry_safe(mmbuf_node, _node_, g_mmbuf_list, list_node) {
			HISI_FB_DEBUG("mmbuf_node_addr(0x%x), mmbuf_node_size(0x%x)!\n", mmbuf_node->addr,
					mmbuf_node->size);
		}
		HISI_FB_INFO("note: mmbuf not enough,addr=0x%x\n", addr);
	} else {
		//node
		node = kzalloc(sizeof(struct hisifb_mmbuf), GFP_KERNEL);
		if (node) {
			node->addr = addr;
			node->size = size;
			list_add_tail(&node->list_node, g_mmbuf_list);
		} else {
			HISI_FB_ERR("kzalloc struct hisifb_mmbuf fail!\n");
		}

		if ((addr & (MMBUF_ADDR_ALIGN - 1)) || (size & (MMBUF_ADDR_ALIGN - 1))) {
			HISI_FB_ERR("addr(0x%x) is not %d bytes aligned, or size(0x%x) is not %d bytes"
				"aligned!\n", addr, MMBUF_ADDR_ALIGN, size, MMBUF_ADDR_ALIGN);

			list_for_each_entry_safe(mmbuf_node, _node_, g_mmbuf_list, list_node) {
				HISI_FB_ERR("mmbuf_node_addr(0x%x), mmbuf_node_size(0x%x)!\n", mmbuf_node->addr,
					mmbuf_node->size);
			}
		}
	}

	up(&hisi_dss_mmbuf_sem);

	if (g_enable_mmbuf_debug) {
		HISI_FB_DEBUG("addr=0x%x, size=%d.\n", addr, size);
	}
	return addr;
}

void hisi_dss_mmbuf_free(void *handle, uint32_t addr, uint32_t size)
{
	struct hisifb_mmbuf *node, *_node_;

	if (NULL == handle) {
		HISI_FB_ERR("handle is NULL!\n");
		return ;
	}

	if (NULL == g_mmbuf_list) {
		HISI_FB_ERR("g_mmbuf_list is NULL!\n");
		return ;
	}

	down(&hisi_dss_mmbuf_sem);

	list_for_each_entry_safe(node, _node_, g_mmbuf_list, list_node) {
		if ((node->addr == addr) && (node->size == size)) {
			gen_pool_free(handle, addr, size);
			list_del(&node->list_node);
			kfree(node);
		}
	}

	up(&hisi_dss_mmbuf_sem);

	if (g_enable_mmbuf_debug) {
		HISI_FB_DEBUG("addr=0x%x, size=%d.\n", addr, size);
	}
}

dss_mmbuf_info_t* hisi_dss_mmbuf_info_get(struct hisi_fb_data_type *hisifd, int idx)
{
	if (NULL == hisifd) {
		HISI_FB_ERR("hisifd is NULL");
		return NULL;
	}
	if ((idx < 0) || (idx >= HISI_DSS_CMDLIST_DATA_MAX)) {
		HISI_FB_ERR("idx is invalid");
		return NULL;
	}

	return &(hisifd->mmbuf_infos[idx]);
}

void hisi_dss_mmbuf_info_clear(struct hisi_fb_data_type *hisifd, int idx)
{
	int i = 0;
	dss_mmbuf_info_t *mmbuf_info = NULL;

	if (NULL == hisifd) {
		HISI_FB_ERR("hisifd is NULL");
		return;
	}
	if ((idx < 0) || (idx >= HISI_DSS_CMDLIST_DATA_MAX)) {
		HISI_FB_ERR("idx is invalid");
		return;
	}

	//mmbuf
	mmbuf_info = &(hisifd->mmbuf_infos[idx]);
	for (i = 0; i < DSS_CHN_MAX_DEFINE; i++) {
		if (mmbuf_info->mm_used[i] == 1) {
			hisi_dss_mmbuf_free(g_mmbuf_gen_pool, mmbuf_info->mm_base[i], mmbuf_info->mm_size[i]);

			if (g_debug_ovl_online_composer) {
				HISI_FB_INFO("fb%d, mm_base(0x%x, %d).\n",
					hisifd->index, mmbuf_info->mm_base[i], mmbuf_info->mm_size[i]);
			}

			mmbuf_info->mm_base[i] = 0;
			mmbuf_info->mm_size[i] = 0;
			mmbuf_info->mm_used[i] = 0;
		}
	}
}

void hisi_mmbuf_info_get_online(struct hisi_fb_data_type *hisifd)
{
	int tmp = 0;

	if (NULL == hisifd) {
		HISI_FB_ERR("hisifd is NULL");
		return;
	}

	tmp = (hisifd->frame_count + 1) % HISI_DSS_CMDLIST_DATA_MAX;
	hisi_dss_mmbuf_info_clear(hisifd, tmp);

	tmp = hisifd->frame_count % HISI_DSS_CMDLIST_DATA_MAX;
	hisifd->mmbuf_info = &(hisifd->mmbuf_infos[tmp]);
}


/*******************************************************************************
**
*/
void hisi_dss_mmbuf_on(struct hisi_fb_data_type *hisifd)
{
	if (NULL == hisifd) {
		HISI_FB_ERR("hisifd is NULL");
		return;
	}

	if (g_dss_version_tag == FB_ACCEL_DSSV501) {
		return;
	}
	if (g_dss_version_tag == FB_ACCEL_DSSV510) {
		return;
	}

	if (g_fpga_flag == 1) {
       //config according to set_pu_mmbuf
	       //step 1: module mtcmos on
		outp32(hisifd->sctrl_base + 0x60, 0x8);  //mmbuf regulator enable
		udelay(200);     //at least 100us
	       //step 2: module clk enable
		outp32(hisifd->sctrl_base + 0x258, 0x00c000c0);  //crg clock enable
		outp32(hisifd->sctrl_base + 0x170, 0x03c00000);
		udelay(2);     //at least 1us
  	       //step 3: module clk disable
		outp32(hisifd->sctrl_base + 0x174, 0x03c00000);
		outp32(hisifd->sctrl_base + 0x258, 0x00c00000);

  	       //step 4: module iso disable
		outp32(hisifd->sctrl_base + 0x44, 0x00000008);
  	       //step 5: memory repair
  	       //step 6: module unrst
		outp32(hisifd->sctrl_base + 0x210, 0x00001800);

  	       //step 7: module clk enable
		outp32(hisifd->sctrl_base + 0x258, 0x00c000c0);
		outp32(hisifd->sctrl_base + 0x170, 0x03c00000);
  	       //step 8: bus idle clear
	}

}

static int hisi_overlay_fastboot(struct hisi_fb_data_type *hisifd)
{
	dss_overlay_t *pov_req_prev = NULL;
	dss_overlay_block_t *pov_h_block_infos = NULL;
	dss_overlay_block_t *pov_h_block = NULL;
	dss_layer_t *layer = NULL;

	if (NULL == hisifd) {
		HISI_FB_ERR("hisifd is NULL");
		return -EINVAL;
	}

	HISI_FB_DEBUG("fb%d, +\n", hisifd->index);

	if (hisifd->index == PRIMARY_PANEL_IDX) {
		pov_req_prev = &(hisifd->ov_req_prev);
		memset(pov_req_prev, 0, sizeof(dss_overlay_t));
		pov_req_prev->ov_block_infos_ptr = (uint64_t)(&(hisifd->ov_block_infos_prev));
		pov_req_prev->ov_block_nums = 1;
		pov_req_prev->ovl_idx = DSS_OVL0;
		pov_req_prev->release_fence = -1;
		pov_req_prev->retire_fence = -1;

		pov_h_block_infos = (dss_overlay_block_t *)pov_req_prev->ov_block_infos_ptr;
		pov_h_block = &(pov_h_block_infos[0]);
		pov_h_block->layer_nums = 1;

		layer = &(pov_h_block->layer_infos[0]);
		layer->img.mmu_enable = 0;
		layer->layer_idx = 0x0;
		layer->chn_idx = DSS_RCHN_D0;
		layer->need_cap = 0;

		memcpy(&(hisifd->dss_module_default.rdma[DSS_RCHN_D0]), &(hisifd->dss_module_default.rdma[DSS_RCHN_D3]),
			sizeof(dss_rdma_t));
		memcpy(&(hisifd->dss_module_default.dfc[DSS_RCHN_D0]), &(hisifd->dss_module_default.dfc[DSS_RCHN_D3]),
			sizeof(dss_dfc_t));
		memcpy(&(hisifd->dss_module_default.ov[DSS_OVL0].ovl_layer[0]), &(hisifd->dss_module_default.ov[DSS_OVL0].ovl_layer[1]),
			sizeof(dss_ovl_layer_t));

		memset(&(hisifd->dss_module_default.mctl_ch[DSS_RCHN_D0]), 0, sizeof(dss_mctl_ch_t));
		memset(&(hisifd->dss_module_default.mctl[DSS_OVL0]), 0, sizeof(dss_mctl_t));

		hisifd->dss_module_default.mctl_sys.chn_ov_sel[DSS_OVL0] = 0xFFFFFFFF;
		hisifd->dss_module_default.mctl_sys.ov_flush_en[DSS_OVL0] = 0x0;

		if (is_mipi_cmd_panel(hisifd)) {
			if (hisifd->vactive0_start_flag == 0) {
				hisifd->vactive0_start_flag = 1;
				hisifd->vactive0_end_flag = 1;
			}
		}
	}

	HISI_FB_DEBUG("fb%d, -\n", hisifd->index);

	return 0;
}

int hisi_overlay_on(struct hisi_fb_data_type *hisifd, bool fastboot_enable)
{
	int ret = 0;
	int ovl_idx = 0;
	int mctl_idx = 0;
	uint32_t cmdlist_idxs = 0;
	int enable_cmdlist = 0;
	static uint32_t index = 0;
	struct hisi_fb_panel_data *pdata = NULL;

	if (NULL == hisifd) {
		HISI_FB_ERR("hisifd is NULL");
		return -EINVAL;
	}

	HISI_FB_DEBUG("fb%d, +\n", hisifd->index);

	memset(&(hisifd->sbl), 0, sizeof(dss_sbl_t));
	hisifd->sbl_enable = 0;
	hisifd->sbl_lsensor_value = 0;
	hisifd->sbl_level = 0;

	hisifd->vactive0_start_flag = 0;
	hisifd->vactive0_end_flag = 0;
	hisifd->crc_flag = 0;
	hisifd->dirty_region_updt.x = 0;
	hisifd->dirty_region_updt.y = 0;
	hisifd->dirty_region_updt.w = hisifd->panel_info.xres;
	hisifd->dirty_region_updt.h = hisifd->panel_info.yres;
	hisifd->resolution_rect.x = 0;
	hisifd->resolution_rect.y = 0;
	hisifd->resolution_rect.w = hisifd->panel_info.xres;
	hisifd->resolution_rect.h = hisifd->panel_info.yres;

	hisifb_dss_overlay_info_init(&hisifd->ov_req);
	hisifd->ov_req.frame_no = 0xFFFFFFFF;
	memset(&hisifd->acm_ce_info, 0, sizeof(hisifd->acm_ce_info));
	memset(&hisifd->prefix_ce_info, 0, sizeof(hisifd->prefix_ce_info));

	g_offline_cmdlist_idxs = 0;

	if ((hisifd->index == PRIMARY_PANEL_IDX) ||
		(hisifd->index == EXTERNAL_PANEL_IDX)) {
		hisifb_activate_vsync(hisifd);
	}

	if (g_dss_module_resource_initialized == 0) {
		hisi_dss_module_default(hisifd);
		g_dss_module_resource_initialized = 1;
		hisifd->dss_module_resource_initialized = true;
		index = hisifd->index;
	} else {
		if (!hisifd->dss_module_resource_initialized) {
			if (hisifd->index == MEDIACOMMON_PANEL_IDX) {
			} else {
				if (hisifd->index != index) {
					if (hisifd_list[index]) {
						memcpy(&(hisifd->dss_module_default),
							&(hisifd_list[index]->dss_module_default), sizeof(dss_module_reg_t));
					}
				}
			}
			hisifd->dss_module_resource_initialized = true;
		}
	}

	enable_cmdlist = g_enable_ovl_cmdlist_online;
	//dss on
	hisifb_dss_on(hisifd, enable_cmdlist);

	if ((hisifd->index == PRIMARY_PANEL_IDX) ||
		(hisifd->index == EXTERNAL_PANEL_IDX)) {
		if (hisifd->index == PRIMARY_PANEL_IDX) {
			ovl_idx = DSS_OVL0;
			mctl_idx = DSS_MCTL0;
		} else {
			ovl_idx = DSS_OVL1;
			mctl_idx = DSS_MCTL1;
		}

		if ((hisifd->index == EXTERNAL_PANEL_IDX) && hisifd->panel_info.fake_external)
			enable_cmdlist = 0;

		ldi_data_gate(hisifd, true);

		hisi_dss_mctl_on(hisifd, mctl_idx, enable_cmdlist, fastboot_enable);

		if (fastboot_enable) {
			hisi_overlay_fastboot(hisifd);
		} else {
			ret = hisi_dss_module_init(hisifd);
			if (ret != 0) {
				HISI_FB_ERR("fb%d, failed to hisi_dss_module_init! ret = %d\n", hisifd->index, ret);
				goto err_out;
			}

			if (hisifd->aod_function && is_mipi_cmd_panel(hisifd)) {
				HISI_FB_INFO("AOD is enable, disable base frame \n");
				hisifd->vactive0_start_flag = 1;
				hisifd->vactive0_end_flag = 1;

				pdata = dev_get_platdata(&hisifd->pdev->dev);
				if (pdata && pdata->set_display_region) {
					pdata->set_display_region(hisifd->pdev, &hisifd->dirty_region_updt);
				}

				goto err_out;
			}

			if (enable_cmdlist) {
				hisifd->set_reg = hisi_cmdlist_set_reg;

				hisi_cmdlist_data_get_online(hisifd);

				cmdlist_idxs = (0x1 << (ovl_idx + DSS_CMDLIST_OV0));
				hisi_cmdlist_add_nop_node(hisifd, cmdlist_idxs, 0, 0);
			} else {
				hisifd->set_reg = hisifb_set_reg;

				hisi_dss_mctl_mutex_lock(hisifd, ovl_idx);
			}

			hisifd->ov_req_prev.ovl_idx = ovl_idx;

			ret = hisi_dss_ovl_base_config(hisifd, NULL, NULL, NULL, ovl_idx, 0);
			if (ret != 0) {
				HISI_FB_ERR("fb%d, faile to hisi_dss_ovl_base_config! ret=%d\n", hisifd->index, ret);
				goto err_out;
			}

			ret = hisi_dss_mctl_ov_config(hisifd, NULL, ovl_idx, false, true);
			if (ret != 0) {
				HISI_FB_ERR("fb%d, faile to hisi_dss_mctl_config! ret=%d\n", hisifd->index, ret);
				goto err_out;
			}

			ret = hisi_dss_ov_module_set_regs(hisifd, NULL, ovl_idx, enable_cmdlist, 1, 0, true);
			if (ret != 0) {
				HISI_FB_ERR("fb%d, failed to hisi_dss_module_config! ret = %d\n", hisifd->index, ret);
				goto err_out;
			}

			if (enable_cmdlist) {
				hisi_cmdlist_flush_cache(hisifd, cmdlist_idxs);
				hisi_cmdlist_config_start(hisifd, mctl_idx, cmdlist_idxs, 0);
			} else {
				hisi_dss_mctl_mutex_unlock(hisifd, ovl_idx);
			}

			single_frame_update(hisifd);
			enable_ldi(hisifd);
			hisifb_frame_updated(hisifd);
			hisifd->frame_count++;

			if (g_debug_ovl_cmdlist) {
				hisi_cmdlist_dump_all_node(hisifd, NULL, cmdlist_idxs);
			}
		}
	err_out:
		hisifb_deactivate_vsync(hisifd);
	} else if (hisifd->index == AUXILIARY_PANEL_IDX) {
		enable_cmdlist = g_enable_ovl_cmdlist_offline;

		hisi_dss_mctl_on(hisifd, DSS_MCTL2, enable_cmdlist, 0);
		hisi_dss_mctl_on(hisifd, DSS_MCTL3, enable_cmdlist, 0);
		hisi_dss_mctl_on(hisifd, DSS_MCTL5, enable_cmdlist, 0);
	} else if (hisifd->index == MEDIACOMMON_PANEL_IDX) {
		hisi_media_common_mctl_on(hisifd);
	} else {
		HISI_FB_ERR("fb%d, not supported!", hisifd->index);
	}

	HISI_FB_DEBUG("fb%d, -\n", hisifd->index);

	return 0;
}

int hisi_overlay_off(struct hisi_fb_data_type *hisifd)
{
	int ret = 0;
	int ovl_idx = 0;
	uint32_t cmdlist_pre_idxs = 0;
	uint32_t cmdlist_idxs = 0;
	int enable_cmdlist = 0;
	dss_overlay_t *pov_req_prev = NULL;

	if (NULL == hisifd) {
		HISI_FB_ERR("hisifd is NULL");
		return -EINVAL;
	}

	pov_req_prev = &(hisifd->ov_req_prev);

	HISI_FB_DEBUG("fb%d, +\n", hisifd->index);
	if ((hisifd->index == PRIMARY_PANEL_IDX) ||
		(hisifd->index == EXTERNAL_PANEL_IDX)) {
		hisifb_activate_vsync(hisifd);

		ret = hisi_vactive0_start_config(hisifd, pov_req_prev);
		if (ret != 0) {
			HISI_FB_ERR("fb%d, hisi_vactive0_start_config failed! ret = %d\n", hisifd->index, ret);
			goto err_out;
		}

		if ((hisifd->aod_function == 1) && (hisifd_list[EXTERNAL_PANEL_IDX] && !hisifd_list[EXTERNAL_PANEL_IDX]->panel_power_on)) {
			HISI_FB_INFO("fb%d, aod mode,no base frame when overlay off\n", hisifd->index);
			mdelay(50); //lint !e747
			goto err_out;
		}

		if (hisifd->index == PRIMARY_PANEL_IDX) {
			ovl_idx = DSS_OVL0;
		} else {
			ovl_idx = DSS_OVL1;
		}

		enable_cmdlist = g_enable_ovl_cmdlist_online;
		if ((hisifd->index == EXTERNAL_PANEL_IDX) && hisifd->panel_info.fake_external)
			enable_cmdlist = 0;

		ret = hisi_dss_module_init(hisifd);
		if (ret != 0) {
			HISI_FB_ERR("fb%d, failed to hisi_dss_module_init! ret = %d\n", hisifd->index, ret);
			goto err_out;
		}

		hisifd->resolution_rect.x = 0;
		hisifd->resolution_rect.y = 0;
		hisifd->resolution_rect.w = hisifd->panel_info.xres;
		hisifd->resolution_rect.h = hisifd->panel_info.yres;

		if (enable_cmdlist) {
			hisifd->set_reg = hisi_cmdlist_set_reg;

			hisi_cmdlist_data_get_online(hisifd);

			ret = hisi_cmdlist_get_cmdlist_idxs(pov_req_prev, &cmdlist_pre_idxs, NULL);
			if (ret != 0) {
				HISI_FB_ERR("fb%d, hisi_cmdlist_get_cmdlist_idxs pov_req_prev failed! ret = %d\n", hisifd->index, ret);
				goto err_out;
			}

			cmdlist_idxs = (1 << (DSS_CMDLIST_OV0 + ovl_idx));
			cmdlist_pre_idxs &= (~ (cmdlist_idxs));

			hisi_cmdlist_add_nop_node(hisifd, cmdlist_pre_idxs, 0, 0);
			hisi_cmdlist_add_nop_node(hisifd, cmdlist_idxs, 0, 0);
		} else {
			hisifd->set_reg = hisifb_set_reg;

			hisi_dss_mctl_mutex_lock(hisifd, ovl_idx);
			cmdlist_pre_idxs = ~0;
		}

		hisi_dss_prev_module_set_regs(hisifd, pov_req_prev, cmdlist_pre_idxs, enable_cmdlist, NULL);

		ret = hisi_dss_ovl_base_config(hisifd, NULL, NULL, NULL, ovl_idx, 0);
		if (ret != 0) {
			HISI_FB_ERR("fb%d, faile to hisi_dss_ovl_base_config! ret=%d\n", hisifd->index, ret);
			goto err_out;
		}

		ret = hisi_dss_mctl_ov_config(hisifd, NULL, ovl_idx, false, true);
		if (ret != 0) {
			HISI_FB_ERR("fb%d, faile to hisi_dss_mctl_config! ret=%d\n", hisifd->index, ret);
			goto err_out;
		}

		ret = hisi_dss_post_scf_config(hisifd, NULL);
		if (ret != 0) {
			HISI_FB_ERR("fb%d, failed to hisi_dss_post_scf_config! ret = %d\n", hisifd->index, ret);
			goto err_out;
		}

		ret= hisi_dss_dirty_region_dbuf_config(hisifd, NULL);
		if (ret != 0) {
			HISI_FB_ERR("fb%d, hisi_dss_dirty_region_dbuf_config failed! ret = %d\n", hisifd->index, ret);
			goto err_out;
		}

		ret = hisi_dss_ov_module_set_regs(hisifd, NULL, ovl_idx, enable_cmdlist, 1, 0, true);
		if (ret != 0) {
			HISI_FB_ERR("fb%d, failed to hisi_dss_module_config! ret = %d\n", hisifd->index, ret);
			goto err_out;
		}

		if (enable_cmdlist) {
			hisi_cmdlist_config_stop(hisifd, cmdlist_pre_idxs);

			cmdlist_idxs |= cmdlist_pre_idxs;
			hisi_cmdlist_flush_cache(hisifd, cmdlist_idxs);

			if (g_debug_ovl_cmdlist) {
				hisi_cmdlist_dump_all_node(hisifd, NULL, cmdlist_idxs);
			}

			hisi_cmdlist_config_start(hisifd, ovl_idx, cmdlist_idxs, 0);
		} else {
			hisi_dss_mctl_mutex_unlock(hisifd, ovl_idx);
		}

		if (hisifd->panel_info.dirty_region_updt_support) {
			hisi_dss_dirty_region_updt_config(hisifd, NULL);
		}

		/* cpu config drm layer */
		hisi_drm_layer_online_config(hisifd, pov_req_prev, NULL);

		ldi_data_gate(hisifd, true);

		single_frame_update(hisifd);
		hisifb_frame_updated(hisifd);

		if (!hisi_dss_check_reg_reload_status(hisifd)) {
			mdelay(20);
		}

		ldi_data_gate(hisifd, false);

		if (is_mipi_cmd_panel(hisifd)) {
			hisifd->ldi_data_gate_en = 1;
		}

		hisifd->frame_count++;
	err_out:
		hisifb_deactivate_vsync(hisifd);
	} else if ((hisifd->index == AUXILIARY_PANEL_IDX) || (hisifd->index == MEDIACOMMON_PANEL_IDX)) {
		; //do nothing
	} else {
		HISI_FB_ERR("fb%d, not support !\n", hisifd->index);
		return -EINVAL;
	}

	//dss off
	if (hisifd->index == AUXILIARY_PANEL_IDX) {
		hisifb_dss_off(hisifd, true);
	} else {
		hisifb_dss_off(hisifd, false);
	}

	hisifd->ldi_data_gate_en = 0;
	hisifd->masklayer_maxbacklight_flag = false;

	memset(&(hisifd->ov_block_infos_prev_prev), 0,
		HISI_DSS_OV_BLOCK_NUMS * sizeof(dss_overlay_block_t));
	memset(&(hisifd->ov_block_infos_prev), 0,
		HISI_DSS_OV_BLOCK_NUMS * sizeof(dss_overlay_block_t));
	memset(&(hisifd->ov_block_infos), 0,
		HISI_DSS_OV_BLOCK_NUMS * sizeof(dss_overlay_block_t));

	hisifb_dss_overlay_info_init(&hisifd->ov_req);
	hisifb_dss_overlay_info_init(&hisifd->ov_req_prev);
	hisifb_dss_overlay_info_init(&hisifd->ov_req_prev_prev);

	memset(&hisifd->effect_updated_flag, 0, sizeof(struct dss_module_update));

	HISI_FB_DEBUG("fb%d, -\n", hisifd->index);
	return 0;
}

int hisi_overlay_on_lp(struct hisi_fb_data_type *hisifd)
{
	int mctl_idx = 0;
	int enable_cmdlist = 0;

	enable_cmdlist = g_enable_ovl_cmdlist_online;
	if ((hisifd->index == EXTERNAL_PANEL_IDX) && hisifd->panel_info.fake_external)
		enable_cmdlist = 0;

	if (hisifd->index == PRIMARY_PANEL_IDX) {
		mctl_idx = DSS_MCTL0;
	} else if (hisifd->index == EXTERNAL_PANEL_IDX) {
		mctl_idx = DSS_MCTL1;
	} else {
		HISI_FB_ERR("fb%d, not supported!", hisifd->index);
		return -1;
	}

	//dss on
	hisifb_dss_on(hisifd, enable_cmdlist);

	hisi_dss_mctl_on(hisifd, mctl_idx, enable_cmdlist, 0);

	return 0;
}

int hisi_overlay_off_lp(struct hisi_fb_data_type *hisifd)
{
	if ((hisifd->index != PRIMARY_PANEL_IDX) && (hisifd->index != EXTERNAL_PANEL_IDX)) {
		HISI_FB_ERR("fb%d, not supported!", hisifd->index);
		return -1;
	}

	hisifb_dss_off(hisifd, true);

	return 0;
}

bool hisi_dss_check_reg_reload_status(struct hisi_fb_data_type *hisifd)
{
	mdelay(50);
	(void)hisifd;
	return true;
}

bool hisi_dss_check_crg_sctrl_status(struct hisi_fb_data_type *hisifd)
{
	uint32_t crg_state_check = 0;
	uint32_t sctrl_mmbuf_dss_check = 0;

	if (NULL == hisifd) {
		HISI_FB_ERR("hisifd is NULL");
		return false;
	}

	crg_state_check = inp32(hisifd->peri_crg_base + PERCLKEN3);
	if ((crg_state_check & 0x23000) != 0x23000) {
		HISI_FB_ERR("dss crg_clk_enable failed, crg_state_check = 0x%x\n", crg_state_check);
		return false;
	}

	crg_state_check = inp32(hisifd->peri_crg_base + PERRSTSTAT3);
	if ((crg_state_check | 0xfffff3ff) != 0xfffff3ff) {
		HISI_FB_ERR("dss crg_reset failed, crg_state_check = 0x%x\n", crg_state_check);
		return false;
	}

	crg_state_check = inp32(hisifd->peri_crg_base + ISOSTAT);
	if ((crg_state_check | 0xffffffbf) != 0xffffffbf) {
		HISI_FB_ERR("dss iso_disable failed, crg_state_check = 0x%x\n", crg_state_check);
		return false;
	}

	crg_state_check = inp32(hisifd->peri_crg_base + PERPWRSTAT);
	if ((crg_state_check & 0x20) != 0x20) {
		HISI_FB_ERR("dss subsys regulator_enabel failed, crg_state_check = 0x%x\n", crg_state_check);
		return false;
	}

	sctrl_mmbuf_dss_check = inp32(hisifd->sctrl_base + SCPERCLKEN1);
	if ((sctrl_mmbuf_dss_check & 0x1000000) != 0x1000000) {
		HISI_FB_ERR("dss subsys mmbuf_dss_clk_enabel failed, sctrl_mmbuf_dss_check = 0x%x\n", sctrl_mmbuf_dss_check);
		return false;
	}

	return true;
}

/*lint -e30 -e142 -e438 -e550*/
int hisi_overlay_ioctl_handler(struct hisi_fb_data_type *hisifd,
	uint32_t cmd, void __user *argp)
{
	int ret = 0;
	uint32_t timediff = 0;
	struct timeval tv0;
	struct timeval tv1;
	struct hisi_panel_info *pinfo = NULL;

	if (NULL == hisifd) {
		HISI_FB_ERR("NULL Pointer\n");
		return - EINVAL;
	}
	pinfo = &(hisifd->panel_info);

	switch (cmd) {
	case HISIFB_OV_ONLINE_PLAY:
		if (hisifd->online_play_count < ONLINE_PLAY_LOG_PRINTF) {
			HISI_FB_INFO("[online_play_count = %d] fb%d  HISIFB_OV_ONLINE_PLAY. \n", hisifd->online_play_count, hisifd->index);
		}
		hisifd->online_play_count++;

		if (hisifd->ov_online_play) {
			if (g_debug_ovl_online_composer_timediff & 0x1)
				hisifb_get_timestamp(&tv0);

			down(&hisifd->blank_sem);
			ret = hisifd->ov_online_play(hisifd, argp);
			if (ret != 0) {
				HISI_FB_ERR("fb%d ov_online_play failed!\n", hisifd->index);
			}
			up(&hisifd->blank_sem);

			if (g_debug_ovl_online_composer_timediff & 0x1) {
				hisifb_get_timestamp(&tv1);
				timediff = hisifb_timestamp_diff(&tv0, &tv1);
				if (timediff >= g_debug_ovl_online_composer_time_threshold)
					HISI_FB_ERR("ONLING_IOCTL_TIMEDIFF is %u us!\n", timediff);
			}

			if (ret == 0) {
				if (hisifd->bl_update) {
					hisifd->bl_update(hisifd);
				}
				hisifb_display_effect_blc_cabc_update(hisifd);
			}
		}

		break;
	case HISIFB_OV_OFFLINE_PLAY:
		if (hisifd->ov_offline_play) {
			//down(&hisifd->blank_sem);
			ret = hisifd->ov_offline_play(hisifd, argp);
			if (ret != 0) {
				HISI_FB_ERR("fb%d ov_offline_play failed!\n", hisifd->index);
			}
			//up(&hisifd->blank_sem);
		}
		break;
	case HISIFB_OV_COPYBIT_PLAY:   //chicago copybit
		if (hisifd->ov_copybit_play) {
			ret = hisifd->ov_copybit_play(hisifd, argp);
			if (ret != 0) {
				HISI_FB_ERR("fb%d ov_copybit_play failed!\n", hisifd->index);
			}
		}
		break;
	case HISIFB_OV_MEDIA_COMMON_PLAY:
		if (hisifd->ov_media_common_play) {
			ret = hisifd->ov_media_common_play(hisifd, argp);
			if (ret != 0) {
				HISI_FB_ERR("fb%d ov_media_common_play failed!\n", hisifd->index);
			}
		}
		break;
	default:
		break;
	}

	return ret;
}
/*lint +e30 +e142 +e438 +e550*/

int hisi_overlay_init(struct hisi_fb_data_type *hisifd)
{
	char wq_name[128] = {0};

	if (NULL == hisifd) {
		HISI_FB_ERR("hisifd is NULL");
		return -EINVAL;
	}
	if (NULL == hisifd->dss_base) {
		HISI_FB_ERR("hisifd->dss_base is NULL");
		return -EINVAL;
	}

	hisifd->dss_module_resource_initialized = false;

	hisifd->vactive0_start_flag = 0;
	hisifd->vactive0_end_flag = 0;
	init_waitqueue_head(&hisifd->vactive0_start_wq);
	hisifd->ldi_data_gate_en = 0;

	hisifd->crc_flag = 0;

	//hisifd->frame_count = 0;
	hisifd->frame_update_flag = 0;

	hisifd->online_play_count = 0;
	hisifd->masklayer_maxbacklight_flag = false;

	memset(&hisifd->ov_req, 0, sizeof(dss_overlay_t));
	hisifd->ov_req.release_fence = -1;
	hisifd->ov_req.retire_fence = -1;
	memset(&hisifd->ov_req_prev, 0, sizeof(dss_overlay_t));
	hisifd->ov_req_prev.release_fence = -1;
	hisifd->ov_req_prev.retire_fence = -1;
	memset(&hisifd->dss_module, 0, sizeof(dss_module_reg_t));
	memset(&hisifd->dss_module_default, 0, sizeof(dss_module_reg_t));

	hisifd->dirty_region_updt.x = 0;
	hisifd->dirty_region_updt.y = 0;
	hisifd->dirty_region_updt.w = hisifd->panel_info.xres;
	hisifd->dirty_region_updt.h = hisifd->panel_info.yres;

	hisifd->resolution_rect.x = 0;
	hisifd->resolution_rect.y = 0;
	hisifd->resolution_rect.w = hisifd->panel_info.xres;
	hisifd->resolution_rect.h = hisifd->panel_info.yres;

	hisifd->pan_display_fnc = hisi_overlay_pan_display;
	hisifd->ov_ioctl_handler = hisi_overlay_ioctl_handler;

	hisifd->dss_debug_wq = NULL;
	hisifd->ldi_underflow_wq = NULL;
	hisifd->rch2_ce_end_wq = NULL;
	hisifd->rch4_ce_end_wq = NULL;
	hisifd->dpp_ce_end_wq = NULL;
	hisifd->masklayer_backlight_notify_wq = NULL;
	hisifd->hiace_end_wq = NULL;
	if ((hisifd->index == PRIMARY_PANEL_IDX) ||
		(hisifd->index == EXTERNAL_PANEL_IDX && !hisifd->panel_info.fake_external) ){
		snprintf(wq_name, 128, "fb%d_dss_debug", hisifd->index);
		hisifd->dss_debug_wq = create_singlethread_workqueue(wq_name);
		if (!hisifd->dss_debug_wq) {
			HISI_FB_ERR("fb%d, create dss debug workqueue failed!\n", hisifd->index);
			return -EINVAL;
		}
		INIT_WORK(&hisifd->dss_debug_work, hisi_dss_debug_func);

		snprintf(wq_name, 128, "fb%d_ldi_underflow", hisifd->index);
		hisifd->ldi_underflow_wq = create_singlethread_workqueue(wq_name);
		if (!hisifd->ldi_underflow_wq) {
			HISI_FB_ERR("fb%d, create ldi underflow workqueue failed!\n", hisifd->index);
			return -EINVAL;
		}
		INIT_WORK(&hisifd->ldi_underflow_work, hisi_ldi_underflow_handle_func);

		if (HISI_DSS_SUPPORT_DPP_MODULE_BIT(DPP_MODULE_ACE) && hisifd->panel_info.acm_ce_support) {
			snprintf(wq_name, 128, "fb%d_dpp_ce_end", hisifd->index);
			hisifd->dpp_ce_end_wq = create_singlethread_workqueue(wq_name);
			if (!hisifd->dpp_ce_end_wq) {
				HISI_FB_ERR("fb%d, create dpp ce end workqueue failed!\n", hisifd->index);
				return -EINVAL;
			}
			INIT_WORK(&hisifd->dpp_ce_end_work, hisi_dpp_ace_end_handle_func);
		}


		if (hisifd->panel_info.hiace_support) {
			snprintf(wq_name, 128, "fb%d_hiace_end", hisifd->index);
			hisifd->hiace_end_wq = create_singlethread_workqueue(wq_name);
			if (!hisifd->hiace_end_wq) {
				HISI_FB_ERR("fb%d, create hiace end workqueue failed!\n", hisifd->index);
				return -EINVAL;
			}
			INIT_WORK(&hisifd->hiace_end_work, hisi_dpp_hiace_end_handle_func);
		}
	}

	if (hisifd->index == PRIMARY_PANEL_IDX) {
		hisifd->set_reg = hisi_cmdlist_set_reg;
		hisifd->ov_online_play = hisi_ov_online_play;
		hisifd->ov_offline_play = NULL;
		hisifd->ov_copybit_play = NULL;
		hisifd->ov_media_common_play = NULL;
		hisifd->ov_wb_isr_handler = NULL;
		hisifd->ov_vactive0_start_isr_handler = hisi_vactive0_start_isr_handler;

		hisifd->crc_isr_handler = NULL;

		hisi_effect_init(hisifd);



	} else if (hisifd->index == EXTERNAL_PANEL_IDX) {
		hisifd->set_reg = hisifb_set_reg;
		hisifd->ov_online_play = hisi_ov_online_play;
		hisifd->ov_offline_play = NULL;
		hisifd->ov_copybit_play = NULL;
		hisifd->ov_media_common_play = NULL;
		hisifd->ov_wb_isr_handler = NULL;
		hisifd->ov_vactive0_start_isr_handler = hisi_vactive0_start_isr_handler;

		hisifd->crc_isr_handler = NULL;
	} else if (hisifd->index == AUXILIARY_PANEL_IDX) {
		hisifd->set_reg = hisi_cmdlist_set_reg;
		hisifd->ov_online_play = NULL;
		hisifd->ov_offline_play = hisi_ov_offline_play;
		hisifd->ov_copybit_play = hisi_ov_copybit_play;
		hisifd->ov_media_common_play = NULL;
		hisifd->ov_wb_isr_handler = NULL;
		hisifd->ov_vactive0_start_isr_handler = NULL;

		hisifd->crc_isr_handler = NULL;

		if (!hisi_mdc_resource_init(hisifd, g_dss_version_tag)) {
			HISI_FB_INFO("mdc channel manager init success!\n");
		}
	} else if (hisifd->index == MEDIACOMMON_PANEL_IDX) {
		hisifd->set_reg = hisi_cmdlist_set_reg;
		hisifd->ov_online_play = NULL;
		hisifd->ov_offline_play = NULL;
		hisifd->ov_copybit_play = NULL;
		hisifd->ov_media_common_play = NULL;

		hisifd->ov_wb_isr_handler = NULL;
		hisifd->ov_vactive0_start_isr_handler = NULL;

		hisifd->crc_isr_handler = NULL;
	} else {
		HISI_FB_ERR("fb%d not support this device!\n", hisifd->index);
		return -EINVAL;
	}

	if (!(hisifd->index == EXTERNAL_PANEL_IDX && !hisifd->panel_info.fake_external)) {
		if (hisi_cmdlist_init(hisifd)) {
			HISI_FB_ERR("fb%d hisi_cmdlist_init failed!\n", hisifd->index);
			return -EINVAL;
		}
	}

	//mmbuf init
	hisi_dss_mmbuf_init(hisifd);

	return 0;
}

int hisi_overlay_deinit(struct hisi_fb_data_type *hisifd)
{
	if (NULL == hisifd) {
		HISI_FB_ERR("hisifd is NULL");
		return -EINVAL;
	}

	if (hisifd->index == PRIMARY_PANEL_IDX) {
		hisi_effect_deinit(hisifd);
	}

	if (hisifd->rch4_ce_end_wq) {
		destroy_workqueue(hisifd->rch4_ce_end_wq);
		hisifd->rch4_ce_end_wq = NULL;
	}

	if (hisifd->rch2_ce_end_wq) {
		destroy_workqueue(hisifd->rch2_ce_end_wq);
		hisifd->rch2_ce_end_wq = NULL;
	}

	if (hisifd->dpp_ce_end_wq) {
		destroy_workqueue(hisifd->dpp_ce_end_wq);
		hisifd->dpp_ce_end_wq = NULL;
	}

	if (hisifd->hiace_end_wq) {
		destroy_workqueue(hisifd->hiace_end_wq);
		hisifd->hiace_end_wq = NULL;
	}

	if (hisifd->dss_debug_wq) {
		destroy_workqueue(hisifd->dss_debug_wq);
		hisifd->dss_debug_wq = NULL;
	}

	if (hisifd->ldi_underflow_wq) {
		destroy_workqueue(hisifd->ldi_underflow_wq);
		hisifd->ldi_underflow_wq = NULL;
	}

	//FIXME:
	if (!(hisifd->index == EXTERNAL_PANEL_IDX && !hisifd->panel_info.fake_external)) {
		hisi_cmdlist_deinit(hisifd);
	}

	//mmbuf deinit
	hisi_dss_mmbuf_deinit(hisifd);


	return 0;
}

void hisi_vactive0_start_isr_handler(struct hisi_fb_data_type *hisifd)
{
	struct hisifb_vsync *vsync_ctrl = NULL;
	ktime_t pre_vactive_timestamp;

	if (NULL == hisifd) {
		HISI_FB_ERR("hisifd is NULL");
		return;
	}

	if (is_mipi_cmd_panel(hisifd) && (hisifd->frame_update_flag == 0)) {
		hisifd->vactive0_start_flag = 1;

		if ((hisifd->secure_ctrl.secure_status == hisifd->secure_ctrl.secure_event) &&
			(hisifd->secure_ctrl.secure_status == DSS_SEC_IDLE) &&
			!hisifb_display_effect_is_need_ace(hisifd)) {
			disable_ldi(hisifd);
		}
	} else {
		hisifd->vactive0_start_flag++;
	}

	wake_up_interruptible_all(&hisifd->vactive0_start_wq);

	if (g_debug_online_vactive) {
		vsync_ctrl = &(hisifd->vsync_ctrl);

		pre_vactive_timestamp = vsync_ctrl->vactive_timestamp;
		vsync_ctrl->vactive_timestamp = ktime_get();

		HISI_FB_INFO("fb%d, VACTIVE =%llu, time_diff=%llu.\n", hisifd->index,
			ktime_to_ns(vsync_ctrl->vactive_timestamp),
			(ktime_to_ns(vsync_ctrl->vactive_timestamp) - ktime_to_ns(pre_vactive_timestamp)));
	}
}
/*lint -e436 -e438 -e527*/

int hisi_vactive0_start_config(struct hisi_fb_data_type *hisifd, dss_overlay_t *pov_req)
{
	int ret = 0;
	int ret1 = 0;
	int times = 0;
	bool panel_check = true;
	uint32_t phy_status = 0;
	uint32_t prev_vactive0_start = 0;
	uint32_t isr_s1 = 0;
	uint32_t isr_s2 = 0;
	uint32_t isr_s2_mask = 0;
	char __iomem *ldi_base = NULL;
	struct timeval tv0;
	struct timeval tv1;
	uint32_t timeout_interval = 0;
	dss_overlay_t *pov_req_dump = NULL;
	dss_overlay_t *pov_req_prev = NULL;
	dss_overlay_t *pov_req_prev_prev = NULL;
	uint32_t cmdlist_idxs = 0;
	uint32_t cmdlist_idxs_prev = 0;
	uint32_t cmdlist_idxs_prev_prev = 0;
	uint32_t read_value[4] = {0};
	uint32_t ldi_vstate = 0;
	uint32_t dmd_index = 0;
	int dmd_ret = 0;
	uint32_t dmd_vactive0_start_flag = 0;
	uint32_t dmd_pov_req_dump_frame_no = 0;
	uint32_t dmd_pov_req_frame_no = 0;
	uint32_t dmd_hisifb_timestamp_diff = 0;
	uint32_t dmd_cmdlist_idxs_prev = 0;
	uint32_t dmd_cmdlist_idxs_prev_prev = 0;
	uint32_t dmd_cmdlist_idxs = 0;
	static u32 s_vactive0_timeout_count = 0;
	int lcd_id = 0;
	if (NULL == hisifd) {
		HISI_FB_ERR("hisifd is NULL");
		return -EINVAL;
	}
	if (NULL == pov_req) {
		HISI_FB_ERR("pov_req is NULL");
		return -EINVAL;
	}

	pov_req_prev = &(hisifd->ov_req_prev);
	pov_req_prev_prev = &(hisifd->ov_req_prev_prev);

	if (g_fpga_flag == 0) {
		timeout_interval = DSS_COMPOSER_TIMEOUT_THRESHOLD_ASIC;
	} else {
		timeout_interval = DSS_COMPOSER_TIMEOUT_THRESHOLD_FPGA;
	}

	if (is_mipi_cmd_panel(hisifd) && (hisifd->frame_update_flag == 0)) {
		//pov_req_dump = &(hisifd->ov_req_prev);
		pov_req_dump = &(hisifd->ov_req_prev_prev);
		if ((hisifd->vactive0_start_flag == 1)
			&& (hisifd->secure_ctrl.secure_event == DSS_SEC_ENABLE)
			&& (pov_req_prev->sec_enable_status == DSS_SEC_ENABLE)){
			hisifd->vactive0_start_flag = 0;
			single_frame_update(hisifd);
		}

		if (hisifd->vactive0_start_flag == 0) {
			hisifb_get_timestamp(&tv0);

		REDO_0:
			ret = wait_event_interruptible_timeout(hisifd->vactive0_start_wq, hisifd->vactive0_start_flag,
				msecs_to_jiffies(timeout_interval));
			if (ret == -ERESTARTSYS) {
				if (times < 50) {
					times++;
					mdelay(10);
					goto REDO_0;
				}
			}
			times = 0;

			if (ret <= 0) {
				hisifb_get_timestamp(&tv1);

				ret1 = hisi_cmdlist_get_cmdlist_idxs(pov_req_prev, &cmdlist_idxs_prev, NULL);
				if (ret1 != 0) {
					HISI_FB_INFO("fb%d, hisi_cmdlist_get_cmdlist_idxs pov_req_prev failed! ret = %d\n", hisifd->index, ret1);
				}

				ret1 = hisi_cmdlist_get_cmdlist_idxs(pov_req_prev_prev, &cmdlist_idxs_prev_prev, NULL);
				if (ret1 != 0) {
					HISI_FB_INFO("fb%d, hisi_cmdlist_get_cmdlist_idxs pov_req_prev_prev failed! ret = %d\n", hisifd->index, ret1);
				}

				cmdlist_idxs = cmdlist_idxs_prev | cmdlist_idxs_prev_prev;
				// cppcheck-suppress *
				HISI_FB_ERR("fb%d, 1wait_for vactive0_start_flag timeout!ret=%d, "
					"vactive0_start_flag=%d, pre_pre_frame_no=%u, frame_no=%u, TIMESTAMP_DIFF is %u us, "
					"cmdlist_idxs_prev=0x%x, cmdlist_idxs_prev_prev=0x%x, cmdlist_idxs=0x%x, itf0_ints=0x%x\n",
					hisifd->index, ret, hisifd->vactive0_start_flag, pov_req_dump->frame_no, pov_req->frame_no,
					hisifb_timestamp_diff(&tv0, &tv1),
					cmdlist_idxs_prev, cmdlist_idxs_prev_prev, cmdlist_idxs,
  					inp32(hisifd->dss_base + DSS_LDI0_OFFSET + LDI_CPU_ITF_INTS)
				);
				HISI_FB_ERR("LDI0_VSTATE = 0x%x.\n", inp32(hisifd->dss_base + DSS_LDI0_OFFSET + LDI_VSTATE));

				s_vactive0_timeout_count++;
				dmd_index = hisifd->index;
				dmd_ret = ret;
				dmd_vactive0_start_flag = hisifd->vactive0_start_flag;
				dmd_pov_req_dump_frame_no = pov_req_dump->frame_no;
				dmd_pov_req_frame_no = pov_req->frame_no;
				dmd_hisifb_timestamp_diff = hisifb_timestamp_diff(&tv0, &tv1);
				dmd_cmdlist_idxs_prev = cmdlist_idxs_prev;
				dmd_cmdlist_idxs_prev_prev = cmdlist_idxs_prev_prev;
				dmd_cmdlist_idxs = cmdlist_idxs;
				lcd_id = hisifb_get_lcd_id(hisifd);

				if (g_debug_ovl_online_composer_hold) {
					dumpDssOverlay(hisifd, pov_req_dump);
					hisi_cmdlist_dump_all_node(hisifd, NULL, cmdlist_idxs);
					mdelay(HISI_DSS_COMPOSER_HOLD_TIME);
				}

				if (g_debug_ldi_underflow_clear && g_ldi_data_gate_en) {
					hisi_cmdlist_config_reset(hisifd, pov_req_dump, cmdlist_idxs);

					ldi_data_gate(hisifd, false);
					mdelay(10);

					panel_check = mipi_panel_check_reg(hisifd, read_value);
					phy_status = inp32(hisifd->mipi_dsi0_base + MIPIDSI_PHY_STATUS_OFFSET);
					ldi_vstate = inp32(hisifd->dss_base + DSS_LDI0_OFFSET + LDI_VSTATE);
					HISI_FB_ERR("fb%d, "
						"Number of the Errors on DSI : 0x05 = 0x%x\n"
						"Display Power Mode : 0x0A = 0x%x\n"
						"Display Signal Mode : 0x0E = 0x%x\n"
						"Display Self-Diagnostic Result : 0x0F = 0x%x\n"
						"LDI vstate : 0x%x, LDI dpi0_hstate : 0x%x\n",
						hisifd->index,
						read_value[0], read_value[1], read_value[2], read_value[3],
						ldi_vstate,
						inp32(hisifd->dss_base + DSS_LDI0_OFFSET + LDI_DPI0_HSTATE));

					memset(&(hisifd->ov_block_infos_prev), 0,
						HISI_DSS_OV_BLOCK_NUMS * sizeof(dss_overlay_block_t));
					hisifb_dss_overlay_info_init(&hisifd->ov_req_prev);

					//waitting te0
					if ((LDI_VSTATE_V_WAIT_TE0 == ldi_vstate) || (!panel_check && (phy_status & BIT(1)))) {
						vactive_timeout_count++;
						if ((vactive_timeout_count >= 3) && hisifd->panel_info.esd_enable) {
							hisifd->esd_recover_state = ESD_RECOVER_STATE_START;
							if (hisifd->esd_ctrl.esd_check_wq) {
								queue_work(hisifd->esd_ctrl.esd_check_wq, &(hisifd->esd_ctrl.esd_check_work));
							}
					    }
					}

					if (s_vactive0_timeout_count > VACTIVE0_TIMEOUT_EXPIRE_COUNT) {
						if (lcd_dclient && !dsm_client_ocuppy(lcd_dclient) && !g_fake_lcd_flag && !hisifd->lcd_self_testing) {
							dsm_client_record(lcd_dclient, "fb%d, 1wait_for vactive0_start_flag timeout!ret=%d, "
								"vactive0_start_flag=%d, pre_pre_frame_no=0x%x, frame_no=0x%x, TIMESTAMP_DIFF is 0x%x, "
								"cmdlist_idxs_prev=0x%x, cmdlist_idxs_prev_prev=0x%x, cmdlist_idxs=0x%x!, "
								"Number of the Errors on DSI : 0x05 = 0x%x, "
								"Display Power Mode : 0x0A = 0x%x, "
								"Display Signal Mode : 0x0E = 0x%x, "
								"Display Self-Diagnostic Result : 0x0F = 0x%x "
								"lcd_id = 0x%x\n",
								dmd_index, dmd_ret, dmd_vactive0_start_flag, dmd_pov_req_dump_frame_no, dmd_pov_req_frame_no,
								dmd_hisifb_timestamp_diff, dmd_cmdlist_idxs_prev, dmd_cmdlist_idxs_prev_prev, dmd_cmdlist_idxs,
								read_value[0], read_value[1], read_value[2], read_value[3], lcd_id);
							dsm_client_notify(lcd_dclient, DSM_LCD_TE_TIME_OUT_ERROR_NO);
						}
						s_vactive0_timeout_count = 0;
					}

					return 0;
				}
				else {
					if (s_vactive0_timeout_count > VACTIVE0_TIMEOUT_EXPIRE_COUNT) {
						if (lcd_dclient && !dsm_client_ocuppy(lcd_dclient) && !g_fake_lcd_flag && !hisifd->lcd_self_testing) {
							dsm_client_record(lcd_dclient, "fb%d, 1wait_for vactive0_start_flag timeout!ret=%d, "
								"vactive0_start_flag=%d, pre_pre_frame_no=0x%x, frame_no=0x%x, TIMESTAMP_DIFF is 0x%x, "
								"cmdlist_idxs_prev=0x%x, cmdlist_idxs_prev_prev=0x%x, cmdlist_idxs=0x%x, lcd_id = 0x%x\n",
								dmd_index, dmd_ret, dmd_vactive0_start_flag, dmd_pov_req_dump_frame_no, dmd_pov_req_frame_no,
								dmd_hisifb_timestamp_diff, dmd_cmdlist_idxs_prev, dmd_cmdlist_idxs_prev_prev, dmd_cmdlist_idxs, lcd_id);
							dsm_client_notify(lcd_dclient, DSM_LCD_TE_TIME_OUT_ERROR_NO);
						}
						s_vactive0_timeout_count = 0;
					}
				}

				ldi_data_gate(hisifd, false);
				mipi_panel_check_reg(hisifd, read_value);
				ldi_vstate = inp32(hisifd->dss_base + DSS_LDI0_OFFSET + LDI_VSTATE);
				HISI_FB_ERR("fb%d, "
					"Number of the Errors on DSI : 0x05 = 0x%x\n"
					"Display Power Mode : 0x0A = 0x%x\n"
					"Display Signal Mode : 0x0E = 0x%x\n"
					"Display Self-Diagnostic Result : 0x0F = 0x%x\n"
					"LDI vstate : 0x%x, LDI dpi0_hstate : 0x%x \n",
					hisifd->index,
					read_value[0], read_value[1], read_value[2], read_value[3],
					ldi_vstate,
					inp32(hisifd->dss_base + DSS_LDI0_OFFSET + LDI_DPI0_HSTATE));

			REDO_1:
				ret = wait_event_interruptible_timeout(hisifd->vactive0_start_wq, hisifd->vactive0_start_flag,
					msecs_to_jiffies(timeout_interval));
				if (ret == -ERESTARTSYS) {
					if (times < 50) {
						times++;
						mdelay(10);
						goto REDO_1;
					}
				}
				times = 0;

				if (ret <= 0) {
					HISI_FB_ERR("fb%d, 2wait_for vactive0_start_flag timeout!ret=%d, "
						"vactive0_start_flag=%d, frame_no=%u.\n",
						hisifd->index, ret, hisifd->vactive0_start_flag, pov_req_dump->frame_no);

					ldi_data_gate(hisifd, false);
					ret = -ETIMEDOUT;
					//waitting te0
					if (LDI_VSTATE_V_WAIT_TE0 == ldi_vstate) {
						vactive_timeout_count++;
						if ((vactive_timeout_count >= 1) && hisifd->panel_info.esd_enable) {
							hisifd->esd_recover_state = ESD_RECOVER_STATE_START;
							if (hisifd->esd_ctrl.esd_check_wq) {
								queue_work(hisifd->esd_ctrl.esd_check_wq, &(hisifd->esd_ctrl.esd_check_work));
							}
							ret = 0;
					    }
					}
				} else {
					ldi_data_gate(hisifd, true);
					ret = 0;
				}
			} else {
				ldi_data_gate(hisifd, true);
				s_vactive0_timeout_count = 0;
				ret = 0;
			}
		}

		ldi_data_gate(hisifd, true);
		hisifd->vactive0_start_flag = 0;
		hisifd->vactive0_end_flag = 0;
		if (ret >= 0) {
			vactive_timeout_count = 0;
		}
	} else {
		pov_req_dump = &(hisifd->ov_req_prev);

		hisifb_get_timestamp(&tv0);
		ldi_data_gate(hisifd, false);
		prev_vactive0_start = hisifd->vactive0_start_flag;

	REDO_2:
		ret = wait_event_interruptible_timeout(hisifd->vactive0_start_wq,
			(prev_vactive0_start != hisifd->vactive0_start_flag),
			msecs_to_jiffies(timeout_interval));
		if (ret == -ERESTARTSYS) {
			if (times < 50) {
				times++;
				mdelay(10);
				goto REDO_2;
			}
		}

		if (g_fastboot_enable_flag && g_enable_ovl_cmdlist_online)
			set_reg(hisifd->dss_base + DSS_MCTRL_CTL0_OFFSET + MCTL_CTL_TOP, 0x1, 32, 0);

		if (ret <= 0) {
			hisifb_get_timestamp(&tv1);

			ret = hisi_cmdlist_get_cmdlist_idxs(pov_req_dump, &cmdlist_idxs, NULL);
			if (ret != 0) {
				HISI_FB_INFO("fb%d, hisi_cmdlist_get_cmdlist_idxs pov_req_prev failed! ret = %d\n", hisifd->index, ret);
			}

			HISI_FB_ERR("fb%d, 1wait_for vactive0_start_flag timeout!ret=%d, "
				"vactive0_start_flag=%d, frame_no=%u, TIMESTAMP_DIFF is %u us,"
				"cmdlist_idxs=0x%x!\n",
				hisifd->index, ret, hisifd->vactive0_start_flag, pov_req_dump->frame_no,
				hisifb_timestamp_diff(&tv0, &tv1), cmdlist_idxs);
			s_vactive0_timeout_count++;
			if (s_vactive0_timeout_count >= VACTIVE0_TIMEOUT_EXPIRE_COUNT) {
				if (lcd_dclient && !dsm_client_ocuppy(lcd_dclient)) {
					dsm_client_record(lcd_dclient, "fb%d, 1wait_for vactive0_start_flag timeout!ret = %d, "
						"vactive0_start_flag = %d, frame_no = %u, TIMESTAMP_DIFF is %u us,"
						"cmdlist_idxs = 0x%x!\n",
						hisifd->index, ret, hisifd->vactive0_start_flag, pov_req_dump->frame_no,
						hisifb_timestamp_diff(&tv0, &tv1), cmdlist_idxs);
					dsm_client_notify(lcd_dclient, DSM_LCD_VACTIVE_TIMEOUT_ERROR_NO);
				}
				s_vactive0_timeout_count = 0;
			}
			if (g_debug_ovl_online_composer_hold) {
				dumpDssOverlay(hisifd, pov_req_dump);
				hisi_cmdlist_dump_all_node(hisifd, NULL, cmdlist_idxs);
				mdelay(HISI_DSS_COMPOSER_HOLD_TIME);
			}
			// for blank display of video mode
			mipi_dsi_reset(hisifd);

			ret = 0;
		} else {
			s_vactive0_timeout_count = 0;
			ret = 0;
		}
	}

	if (ret == -ETIMEDOUT) {
		if (pov_req_dump && pov_req_dump->ovl_idx == DSS_OVL0) {
			isr_s1 = inp32(hisifd->dss_base + GLB_CPU_PDP_INTS);
			isr_s2_mask = inp32(hisifd->dss_base + DSS_LDI0_OFFSET + LDI_CPU_ITF_INT_MSK);
			isr_s2 = inp32(hisifd->dss_base + DSS_LDI0_OFFSET + LDI_CPU_ITF_INTS);
			ldi_base = hisifd->dss_base + DSS_LDI0_OFFSET;
			HISI_FB_ERR("fb%d, isr_s1=0x%x, isr_s2_mask=0x%x, isr_s2=0x%x, "
						"LDI_CTRL(0x%x), LDI_FRM_MSK(0x%x).\n",
						hisifd->index, isr_s1, isr_s2_mask, isr_s2,
						inp32(ldi_base + LDI_CTRL),
						inp32(ldi_base + LDI_FRM_MSK));
		} else if (pov_req_dump && pov_req_dump->ovl_idx == DSS_OVL1) {
			ldi_base = hisifd->dss_base + DSS_LDI1_OFFSET;

			isr_s1 = inp32(hisifd->dss_base + GLB_CPU_SDP_INTS);
			isr_s2_mask = inp32(ldi_base + LDI_CPU_ITF_INT_MSK);
			isr_s2 = inp32(ldi_base + LDI_CPU_ITF_INTS);

			HISI_FB_ERR("fb%d, isr_s1=0x%x, isr_s2_mask=0x%x, isr_s2=0x%x, "
						"LDI_CTRL(0x%x), LDI_FRM_MSK(0x%x).\n",
						hisifd->index, isr_s1, isr_s2_mask, isr_s2,
						inp32(ldi_base + LDI_CTRL),
						inp32(ldi_base + LDI_FRM_MSK));
		} else {
			; //do nothing;
		}
	}

	return ret;
}

static int hisi_crc_get_result(struct hisi_fb_data_type *hisifd,
	dss_overlay_t *pov_req)
{
	if (NULL == hisifd) {
		HISI_FB_ERR("hisifd is NULL");
		return -EINVAL;
	}
	if (NULL == pov_req) {
		HISI_FB_ERR("pov_req is NULL");
		return -EINVAL;
	}

	if (hisifd->index == PRIMARY_PANEL_IDX) {
		if (pov_req->crc_enable_status == DSS_CRC_OV_EN) {
			pov_req->crc_info.crc_ov_result = inp32(hisifd->dss_base + DSS_DBG_OFFSET + DBG_CRC_DBG_OV0);
			pov_req->crc_info.crc_ov_frm = inp32(hisifd->dss_base + DSS_DBG_OFFSET + DBG_CRC_OV0_FRM);
		} else if (pov_req->crc_enable_status == DSS_CRC_LDI_EN) {
			pov_req->crc_info.crc_ldi_result = inp32(hisifd->dss_base + GLB_CRC_DBG_LDI0);
			pov_req->crc_info.crc_ldi_frm = inp32(hisifd->dss_base + GLB_CRC_LDI0_FRM);
		} else if (pov_req->crc_enable_status == DSS_CRC_SUM_EN) {
			pov_req->crc_info.crc_ldi_result = inp32(hisifd->dss_base + DSS_DBG_OFFSET + DBG_CRC_DBG_SUM);
			pov_req->crc_info.crc_ldi_frm = inp32(hisifd->dss_base + DSS_DBG_OFFSET + DBG_CRC_SUM_FRM);
		}
	} else if (hisifd->index == EXTERNAL_PANEL_IDX) {
		if (pov_req->crc_enable_status == DSS_CRC_OV_EN) {
			pov_req->crc_info.crc_ov_result = inp32(hisifd->dss_base + DSS_DBG_OFFSET + DBG_CRC_DBG_OV1);
			pov_req->crc_info.crc_ov_frm = inp32(hisifd->dss_base + DSS_DBG_OFFSET + DBG_CRC_OV1_FRM);
		} else if (pov_req->crc_enable_status == DSS_CRC_LDI_EN) {
			pov_req->crc_info.crc_ldi_result = inp32(hisifd->dss_base + GLB_CRC_DBG_LDI1);
			pov_req->crc_info.crc_ldi_frm = inp32(hisifd->dss_base + GLB_CRC_LDI1_FRM);
		} else if (pov_req->crc_enable_status == DSS_CRC_SUM_EN) {
			pov_req->crc_info.crc_ldi_result = inp32(hisifd->dss_base + DSS_DBG_OFFSET + DBG_CRC_DBG_SUM);
			pov_req->crc_info.crc_ldi_frm = inp32(hisifd->dss_base + DSS_DBG_OFFSET + DBG_CRC_SUM_FRM);
		}
	} else {
		HISI_FB_ERR("fb%d, not support!", hisifd->index);
		return -EINVAL;
	}

	return 0;
}

void hisi_dss_debug_func(struct work_struct *work)
{
	struct hisi_fb_data_type *hisifd = NULL;

	hisifd = container_of(work, struct hisi_fb_data_type, dss_debug_work);
	if (hisifd == NULL) {
		HISI_FB_ERR("hisifd is NULL point!\n");
		return;
	}

	dumpDssOverlay(hisifd, &hisifd->ov_req);
}

void hisi_ldi_underflow_handle_func(struct work_struct *work)
{
	struct hisi_fb_data_type *hisifd = NULL;
	dss_overlay_t *pov_req_prev = NULL;
	dss_overlay_t *pov_req_prev_prev = NULL;
	uint32_t cmdlist_idxs_prev = 0;
	uint32_t cmdlist_idxs_prev_prev = 0;
	char __iomem *ldi_base = NULL;
	int ret = 0;
	uint32_t isr_s1 = 0;
	uint32_t isr_s2 = 0;

	hisifd = container_of(work, struct hisi_fb_data_type, ldi_underflow_work);
	if (hisifd == NULL) {
		HISI_FB_ERR("hisifd is NULL point!\n");
		return;
	}

	HISI_FB_INFO("fb%d, +.\n", hisifd->index);

	down(&hisifd->blank_sem0);
	if (!hisifd->panel_power_on) {
		HISI_FB_INFO("fb%d, panel is power off!", hisifd->index);
		up(&hisifd->blank_sem0);
		return ;
	}

	pov_req_prev = &(hisifd->ov_req_prev);
	pov_req_prev_prev = &(hisifd->ov_req_prev_prev);

	ret = hisi_cmdlist_get_cmdlist_idxs(pov_req_prev, &cmdlist_idxs_prev, NULL);
	if (ret != 0) {
		HISI_FB_ERR("fb%d, hisi_cmdlist_get_cmdlist_idxs pov_req_prev failed! ret = %d\n", hisifd->index, ret);
	}

	ret = hisi_cmdlist_get_cmdlist_idxs(pov_req_prev_prev, &cmdlist_idxs_prev_prev, NULL);
	if (ret != 0) {
		HISI_FB_ERR("fb%d, hisi_cmdlist_get_cmdlist_idxs pov_req_prev_prev failed! ret = %d\n", hisifd->index, ret);
	}


	hisifb_activate_vsync(hisifd);

	hisi_cmdlist_config_reset(hisifd, pov_req_prev, cmdlist_idxs_prev | cmdlist_idxs_prev_prev);

	enable_ldi(hisifd);

	if (hisifd->index == PRIMARY_PANEL_IDX) {
		isr_s1 = inp32(hisifd->dss_base + GLB_CPU_PDP_INTS);
		isr_s2 = inp32(hisifd->dss_base + DSS_LDI0_OFFSET + LDI_CPU_ITF_INTS);
		outp32(hisifd->dss_base + DSS_LDI0_OFFSET + LDI_CPU_ITF_INTS, isr_s2);
		outp32(hisifd->dss_base + GLB_CPU_PDP_INTS, isr_s1);

	} else if (hisifd->index == EXTERNAL_PANEL_IDX) {
		isr_s1 = inp32(hisifd->dss_base + GLB_CPU_SDP_INTS);
		ldi_base = hisifd->dss_base + DSS_LDI1_OFFSET;
		isr_s2 = inp32(ldi_base + LDI_CPU_ITF_INTS);
		outp32(ldi_base + LDI_CPU_ITF_INTS, isr_s2);
		outp32(hisifd->dss_base + GLB_CPU_SDP_INTS, isr_s1);

	}

	hisifb_deactivate_vsync(hisifd);

	up(&hisifd->blank_sem0);

	HISI_FB_INFO("fb%d, -. cmdlist_idxs_prev = 0x%x, cmdlist_idxs_prev_prev = 0x%x\n", hisifd->index, cmdlist_idxs_prev, cmdlist_idxs_prev_prev);
}

/*lint +e436 +e438 +e527*/
/*lint +e778 +e732 +845 +774 +e438*/

