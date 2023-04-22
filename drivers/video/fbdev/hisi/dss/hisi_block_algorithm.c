/*
* Copyright (c) 2013-2014, Hisilicon Tech. Co., Ltd. All rights reserved.
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
#include "hisi_block_algorithm.h"
#include "hisi_overlay_utils.h"



#define SCF_INPUT_OV  (16)

#define WDMA_ROT_LINEBUF	(480)
#define AFBCE_LINEBUF	(480)
#define HFBCE_LINEBUF	(512)

#define RCHN_V2_SCF_LINE_BUF (512)
#define SHARPNESS_LINE_BUF	(2560)

#define WCH_SCF_LINE_BUF (512)

#define MAX_OFFLINE_SCF 4
#define MAX_OFFLINE_LAYER_NUMBER 8
#define BLOCK_SIZE_INVALID	(0xFFFF)


int rect_across_rect(dss_rect_t rect1, dss_rect_t rect2, dss_rect_t *cross_rect)
{
	uint32_t center_x = 0;
	uint32_t center_y = 0;

	if (NULL == cross_rect) {
		HISI_FB_ERR("cross_rect is NULL");
		return -EINVAL;
	}

	memset(cross_rect, 0x0, sizeof(dss_rect_t));

	if (rect1.w == 0 || rect1.h == 0 || rect2.w == 0 || rect2.h == 0)
		return 0;

	center_x = abs(rect2.x + rect2.w - 1 + rect2.x - (rect1.x + rect1.w - 1 + rect1.x));
	center_y = abs(rect2.y + rect2.h - 1 + rect2.y - (rect1.y + rect1.h - 1 + rect1.y));

	if ((center_x < rect2.w + rect1.w) && (center_y < rect2.h + rect1.h)) {
		// rect cross case
		cross_rect->x = MAX(rect1.x, rect2.x);
		cross_rect->y = MAX(rect1.y, rect2.y);
		cross_rect->w = MIN(rect1.x + rect1.w - 1,rect2.x + rect2.w - 1) - cross_rect->x + 1;
		cross_rect->h = MIN(rect1.y + rect1.h - 1,rect2.y + rect2.h - 1) - cross_rect->y + 1;

		return 1;
	}

	return 0;
}

uint32_t calc_dest_block_size(dss_overlay_t *pov_req, dss_overlay_block_t *pov_h_block)
{
	uint32_t i = 0;
	uint32_t block_width = BLOCK_SIZE_INVALID;
	int32_t scf_line_buffer = SCF_LINE_BUF;
	dss_layer_t *layer = NULL;
	dss_wb_layer_t *wb_layer = NULL;

	if (NULL == pov_req) {
		HISI_FB_ERR("pov_req is NULL");
		return BLOCK_SIZE_INVALID;
	}
	if (NULL == pov_h_block) {
		HISI_FB_ERR("pov_h_block is NULL");
		return BLOCK_SIZE_INVALID;
	}

	for (i = 0; i < pov_h_block->layer_nums; i++) {
		layer = &(pov_h_block->layer_infos[i]);

		if (layer->need_cap & (CAP_DIM | CAP_BASE))
			continue;

		/* sharpenss line buffer is 1600 for austin and dallas, but 2560 for chicago */
		if ((layer->need_cap & CAP_2D_SHARPNESS) &&
			(layer->src_rect.w > SHARPNESS_LINE_BUF)) {
			block_width = MIN(block_width, SHARPNESS_LINE_BUF);
		}

		if (layer->need_cap & CAP_AFBCD) {
			block_width = MIN(block_width, AFBCE_LINEBUF);
		}

		if (layer->need_cap & CAP_HFBCD) {
			block_width = MIN(block_width, HFBCE_LINEBUF);
		}

		/* scaler line buffer, default value is 2560, but line buffer of rchn_v2 is 512,
		scaler line buffer should be subtracted by 32 according to scale algorithm*/
		if (layer->chn_idx == DSS_RCHN_V2) {
			scf_line_buffer = RCHN_V2_SCF_LINE_BUF;
		} else {
			scf_line_buffer = SCF_LINE_BUF;
		}

		scf_line_buffer = scf_line_buffer - 32;

		if (layer->src_rect.h != layer->dst_rect.h) {
			if (((layer->src_rect.w >= layer->dst_rect.w) && (layer->dst_rect.w > scf_line_buffer)) ||
				((layer->src_rect.w < layer->dst_rect.w) && (layer->src_rect.w > scf_line_buffer))) {
				block_width = MIN(block_width, scf_line_buffer);
			}
		}
	}

	for (i = 0; i < pov_req->wb_layer_nums; i++) {
		wb_layer = &(pov_req->wb_layer_infos[i]);

		if (wb_layer->transform & HISI_FB_TRANSFORM_ROT_90) {
			/* maximum of hfbce linebuffer is 512*/
			if (wb_layer->need_cap & CAP_HFBCE) {
				block_width = MIN(block_width, HFBCE_LINEBUF);
			} else {
				/* maximum of rot linebuffer is 480 */
				block_width = MIN(block_width, WDMA_ROT_LINEBUF);
			}
		}

		/* maximum of afbce linebuffer is 480*/
		if (wb_layer->need_cap & CAP_AFBCE) {
			block_width = MIN(block_width, AFBCE_LINEBUF);
		}

		/* maximum of hfbce linebuffer is 512*/
		if (wb_layer->need_cap & CAP_HFBCE) {
			block_width = MIN(block_width, HFBCE_LINEBUF);
		}

		/* maximum of wch scl linebuffer is 512*/
		if (wb_layer->need_cap & CAP_SCL) {
			block_width = MIN(block_width, WCH_SCF_LINE_BUF);
		}
	}

	return block_width;
}

int scf_output_suitable(uint32_t x_start, uint32_t x_end, uint32_t pos)
{
	if ((x_start > pos) || (x_end < pos))
		return 0;

	/* if distance between layer start/end and pos, return 1 for adjust */
	if ((pos - x_start < SCF_MIN_OUTPUT) || (x_end - pos + 1 < SCF_MIN_OUTPUT))
		return 1;

	return 0;
}

int block_fix_scf_constraint(dss_overlay_t *pov_req, dss_overlay_block_t *pov_h_block,
	uint32_t block_size, uint32_t end_pos, uint32_t *fix_size)
{
	uint32_t i = 0;
	uint32_t end = end_pos;
	uint32_t scf_layer_num = 0;
	dss_rect_t scf_dst_rect[MAX_OFFLINE_LAYER_NUMBER];
	dss_layer_t *layer = NULL;

	if (pov_req == NULL) {
		HISI_FB_ERR("pov_req is NULL point.\n");
		return -1;
	}

	if (pov_h_block == NULL) {
		HISI_FB_ERR("pov_h_block is NULL point.\n");
		return -1;
	}

	if (fix_size == NULL) {
		HISI_FB_ERR("fix_size is NULL point.\n");
		return -1;
	}

	*fix_size = block_size;

	if (block_size <= SCF_MIN_OUTPUT) {
		HISI_FB_ERR("block size[%d] is too small!\n", block_size);
		return -1;
	}

	for (i = 0; i < pov_h_block->layer_nums; i++) {
		layer = &(pov_h_block->layer_infos[i]);
		if (layer->need_cap & (CAP_BASE | CAP_DIM | CAP_PURE_COLOR)) {
			continue;
		}

		if ((g_dss_version_tag == FB_ACCEL_HI366x) ||
			((g_dss_version_tag & (FB_ACCEL_KIRIN970 | FB_ACCEL_DSSV501 | FB_ACCEL_DSSV510 | FB_ACCEL_DSSV320 | FB_ACCEL_DSSV330))
			&& (layer->need_cap & CAP_2D_SHARPNESS))) {
			if (scf_layer_num >= MAX_OFFLINE_LAYER_NUMBER) {
				HISI_FB_ERR("layer number in offline [%d] is more than scf moudle [%d]\n",
					scf_layer_num, MAX_OFFLINE_LAYER_NUMBER);
				return -1;
			}
		} else {
			if ((layer->src_rect.w == layer->dst_rect.w) && (layer->src_rect.h == layer->dst_rect.h)) {
				continue;
			}

			if (scf_layer_num >= MAX_OFFLINE_SCF) {
				HISI_FB_ERR("scf layer in offline [%d] is more than scf moudle [%d]\n",
					scf_layer_num, MAX_OFFLINE_SCF);
				return -1;
			}
		}

		/* get all scaler layers*/
		scf_dst_rect[scf_layer_num].x = layer->dst_rect.x;
		scf_dst_rect[scf_layer_num].y = layer->dst_rect.y;
		scf_dst_rect[scf_layer_num].w = layer->dst_rect.w;
		scf_dst_rect[scf_layer_num].h = layer->dst_rect.h;
		scf_layer_num++;
	}

	if (scf_layer_num == 0)
		return 0;

REDO:
	for (i = 0; i < scf_layer_num; i++) {
		if (scf_output_suitable(scf_dst_rect[i].x,
			scf_dst_rect[i].x + scf_dst_rect[i].w - 1, pov_req->wb_ov_rect.x + end)) {
			end = end - SCF_MIN_OUTPUT;
			goto REDO;
		}
	}

	*fix_size = block_size - (end_pos - end);
	return 0;
}

int adjust_layers_cap(dss_overlay_t *pov_req, dss_overlay_block_t *pov_h_block, dss_wb_layer_t *wb_layer)
{
	int i = 0;
	int temp = 0;
	dss_layer_t *layer = NULL;
	bool has_rot = false;
	uint32_t stretch_line_num = 0;
	dss_rect_t temp_rect;

	if (NULL == pov_req) {
		HISI_FB_ERR("pov_req is NULL");
		return -EINVAL;
	}

	if (NULL == pov_h_block) {
		HISI_FB_ERR("pov_h_block is NULL");
		return -EINVAL;
	}

	if (NULL == wb_layer) {
		HISI_FB_ERR("wb_layer is NULL");
		return -EINVAL;
	}

	for (i = 0; i < pov_h_block->layer_nums; i++) {
		layer = &pov_h_block->layer_infos[i];

		if (layer->transform & HISI_FB_TRANSFORM_ROT_90) {
			temp = layer->dst_rect.x;
			layer->dst_rect.x = pov_req->wb_ov_rect.x + (layer->dst_rect.y - pov_req->wb_ov_rect.y);
			layer->dst_rect.y = pov_req->wb_ov_rect.y + temp - pov_req->wb_ov_rect.x;

			temp = layer->dst_rect.w;
			layer->dst_rect.w = layer->dst_rect.h;
			layer->dst_rect.h = temp;

			if (layer->transform == HISI_FB_TRANSFORM_ROT_90) {
				layer->transform = HISI_FB_TRANSFORM_FLIP_V;
			} else if (layer->transform == HISI_FB_TRANSFORM_ROT_270) {
				layer->transform = HISI_FB_TRANSFORM_FLIP_H;
			} else if (layer->transform == (HISI_FB_TRANSFORM_ROT_90 | HISI_FB_TRANSFORM_FLIP_H)) {
				layer->transform = HISI_FB_TRANSFORM_ROT_180;
			} else if (layer->transform == (HISI_FB_TRANSFORM_ROT_90 | HISI_FB_TRANSFORM_FLIP_V)) {
				layer->transform = HISI_FB_TRANSFORM_NOP;
			} else {
				; //do nothing
			}

			has_rot = true;
		}
	}

	//FIXME: 2 wb layer
	if (has_rot) {
		for (i = 0; i < pov_req->wb_layer_nums; i++) {
			wb_layer = &(pov_req->wb_layer_infos[i]);
			temp = wb_layer->src_rect.w;
			wb_layer->src_rect.w = wb_layer->src_rect.h;
			wb_layer->src_rect.h = temp;

			wb_layer->transform = (HISI_FB_TRANSFORM_ROT_90 | HISI_FB_TRANSFORM_FLIP_V);
		}
	}

	if ((pov_h_block->layer_nums == 1) && (pov_req->wb_layer_nums == 1) ) {
		if (wb_layer->need_cap & CAP_SCL) {
			layer = &pov_h_block->layer_infos[0];
			stretch_line_num = get_rdma_stretch_line_num(layer);
			temp_rect.w = layer->src_rect.w;
			temp_rect.h = layer->src_rect.h;

			if (stretch_line_num > 0) {
				temp_rect.h = (layer->src_rect.h / stretch_line_num) +
					((layer->src_rect.h % stretch_line_num) ? 1 : 0);
			}

			wb_layer->src_rect.w = temp_rect.w;
			wb_layer->src_rect.h = temp_rect.h;
		}
	}

	return 0;
}

int get_ov_block_rect(dss_overlay_t *pov_req, dss_overlay_block_t *pov_h_block, dss_wb_layer_t *wb_layer,
	int *block_num, dss_rect_t *ov_block_rects[], bool has_wb_scl)
{
	int ret = 0;
	uint32_t block_size = 0xFFFF;
	uint32_t current_offset = 0;
	uint32_t last_offset = 0;
	uint32_t fix_scf_span = 0;
	dss_layer_t *layer = NULL;
	uint32_t i = 0;
	int block_has_layer = 0;
	int w = 0;
	int h = 0;
	dss_rect_t wb_block_rect;

	if (NULL == pov_req) {
		HISI_FB_ERR("pov_req is NULL");
		return -EINVAL;
	}
	if (NULL == pov_h_block) {
		HISI_FB_ERR("pov_h_block is NULL");
		return -EINVAL;
	}
	if (NULL == ov_block_rects) {
		HISI_FB_ERR("ov_block_rects is NULL");
		return -EINVAL;
	}
	if (NULL == block_num) {
		HISI_FB_ERR("block_num is NULL");
		return -EINVAL;
	}
	if (NULL == wb_layer) {
		HISI_FB_ERR("wb_layer is NULL");
		return -EINVAL;
	}

	*block_num = 0;

	/* adjust layer transform cap, source layer dst_rect and writeback layer src_rect */
	adjust_layers_cap(pov_req, pov_h_block, wb_layer);
	if (has_wb_scl) {
		if (wb_layer->transform == (HISI_FB_TRANSFORM_ROT_90 | HISI_FB_TRANSFORM_FLIP_V)) {
			wb_block_rect.x = wb_layer->dst_rect.x;
			wb_block_rect.y = wb_layer->dst_rect.y;
			wb_block_rect.w = wb_layer->dst_rect.h;
			wb_block_rect.h = wb_layer->dst_rect.w;
		} else {
			wb_block_rect = wb_layer->dst_rect;
		}
	} else {
		wb_block_rect = wb_layer->src_rect;
	}

	w = wb_block_rect.w;
	h = wb_block_rect.h;

	/* init block size according to source layer dst_rect */
	block_size = calc_dest_block_size(pov_req, pov_h_block);

	/* if block size is invalid or larger than write back width, block is not needed.
	Then block num is set to 1, and block rect is set to write back layer rect */
	if ((block_size == BLOCK_SIZE_INVALID) || (block_size >= w)) {
		ov_block_rects[*block_num]->x = wb_block_rect.x;
		ov_block_rects[*block_num]->y = wb_block_rect.y;
		ov_block_rects[*block_num]->w = wb_block_rect.w;
		ov_block_rects[*block_num]->h = wb_block_rect.h;

		*block_num = 1;
		return ret;
	}

	current_offset = block_size;
	fix_scf_span = block_size;

	for (current_offset = block_size; last_offset < w; last_offset = current_offset, current_offset += block_size) {
		/* make sure each block of scaler layer is larger than 16 */
		if (!has_wb_scl) {
			if (block_fix_scf_constraint(pov_req, pov_h_block, block_size, current_offset, &fix_scf_span) != 0) {
				HISI_FB_ERR("block_fix_scf_constraint err!\n");
				return -3;
			}
		}

		/* recalculate the block size, the final value */
		current_offset = current_offset - (block_size - fix_scf_span);
		block_has_layer = 0;

		for (i = 0; i < pov_h_block->layer_nums; i++) {
			layer = &(pov_h_block->layer_infos[i]);

			if (((last_offset + pov_req->wb_ov_rect.x) <= (layer->dst_rect.x + layer->dst_rect.w - 1)) &&
				(layer->dst_rect.x < (current_offset + pov_req->wb_ov_rect.x))) {
				block_has_layer = 1;
				if((*block_num) >= HISI_DSS_CMDLIST_BLOCK_MAX)
					return -5;

				/* get the block rectangles */
				ov_block_rects[*block_num]->x = wb_block_rect.x + last_offset;
				ov_block_rects[*block_num]->y = wb_block_rect.y;
				ov_block_rects[*block_num]->w = MIN(current_offset - last_offset, w - last_offset);
				ov_block_rects[*block_num]->h = h;

				(*block_num)++;
				break;
			}
		}

		if (block_has_layer == 0) {
			if((*block_num) >= HISI_DSS_CMDLIST_BLOCK_MAX)
				return -6;

			ov_block_rects[*block_num]->x = wb_block_rect.x + last_offset;
			ov_block_rects[*block_num]->y = wb_block_rect.y;
			ov_block_rects[*block_num]->w = MIN(current_offset - last_offset, w - last_offset);
			ov_block_rects[*block_num]->h = h;

			(*block_num)++;
		}

		if (g_debug_ovl_block_composer) {
			HISI_FB_INFO("ov_block_rects[%d]:[%d:%d:%d:%d], wb_block_rect[%d:%d:%d:%d], current_offset=%d,"
				"fix_scf_span=%d, last_offset=%d, w=%d!\n",
				*block_num, ov_block_rects[*block_num-1]->x, ov_block_rects[*block_num-1]->y,
				ov_block_rects[*block_num-1]->w, ov_block_rects[*block_num-1]->h, wb_block_rect.x, wb_block_rect.y,
				wb_block_rect.w, wb_block_rect.h, current_offset, fix_scf_span, last_offset, w);
		}
	}

	hisifb_adjust_block_rect(*block_num, ov_block_rects, wb_layer);

	return ret;
}
/*lint -e713 -e732 -e737 -e834 -e838 -e845*/
static int create_h_v_block_layer(dss_layer_t *h_layer, dss_layer_t *h_v_layer,
	dss_rect_t dst_cross_rect, dss_rect_t ov_block_rect)
{
	int input_startpos = 0;
	int input_span = 0;
	uint32_t output_startpos = 0; //relative to overlay plane
	uint32_t output_span = 0;
	uint32_t h_ratio = 0;
	uint32_t acc_hscl = 0;
	uint32_t scf_read_start = 0;
	uint32_t scf_read_end = 0;
	dss_rect_t rect_transform = {0};
	dss_rect_t src_rect = {0};
	int scf_int = 0;
	int scf_rem = 0;
	dss_rect_t dst_rect = {0};
	int first_block = 0; //is the first block of this layer
	int last_block = 0; //is the last block of this layer
	int scf_in_start = 0;
	int scf_in_end = 0;
	int chn_idx_temp;
	chn_idx_temp = DSS_RCHN_V0;

	if (NULL == h_layer) {
		HISI_FB_ERR("h_layer is NULL");
		return -EINVAL;
	}
	if (NULL == h_v_layer) {
		HISI_FB_ERR("h_v_layer is NULL");
		return -EINVAL;
	}

	first_block = (h_layer->dst_rect.x >= ov_block_rect.x) ? 1 : 0;
	last_block = ((ov_block_rect.x + ov_block_rect.w) >= (h_layer->dst_rect.x + h_layer->dst_rect.w)) ? 1 : 0;

	output_startpos = dst_cross_rect.x - h_layer->dst_rect.x;
	output_span = dst_cross_rect.w;
	input_startpos = output_startpos;
	input_span = output_span;

	/* handle arsr2p layer */
#define ARSR2P_OVERLAPH 16

	if (h_layer->chn_idx == chn_idx_temp && (h_layer->src_rect.w < h_layer->dst_rect.w
        || h_layer->src_rect.h != h_layer->dst_rect.h || (h_layer->need_cap & CAP_2D_SHARPNESS))) {

		if ((!first_block) && (output_startpos%2)) {  //not the first block and start position is not aligned to 2
			//start position subtracted by 1
			h_v_layer->block_info.arsr2p_left_clip = 1;
			dst_cross_rect.x = dst_cross_rect.x - h_v_layer->block_info.arsr2p_left_clip;
			dst_cross_rect.w = dst_cross_rect.w + h_v_layer->block_info.arsr2p_left_clip;

			output_startpos = output_startpos - h_v_layer->block_info.arsr2p_left_clip;
			output_span = dst_cross_rect.w;
			input_startpos = output_startpos;
			input_span = output_span;
		}

		if (h_layer->src_rect.w > h_layer->dst_rect.w) {
			src_rect.x = h_layer->src_rect.x;
			src_rect.y = h_layer->src_rect.y;
			src_rect.w = h_layer->dst_rect.w; //scaling down in horizental direction, set arsr2p input width to dst width
			src_rect.h = h_layer->src_rect.h;
		} else {
			src_rect = h_layer->src_rect;
		}

		//h_ratio=(arsr_input_width*65536+65536-ihleft)/(arsr_output_width+1)
		h_ratio = (DSS_WIDTH(src_rect.w) * ARSR2P_INC_FACTOR + ARSR2P_INC_FACTOR - acc_hscl) /
			h_layer->dst_rect.w;//lint !e573

		//starti = ceil( starto *((double)(ihinc))/(65536.0) - overlapH
		scf_int = output_startpos * h_ratio/ARSR2P_INC_FACTOR;
		scf_rem = output_startpos * h_ratio%ARSR2P_INC_FACTOR;
		scf_in_start = (scf_rem > 0) ? (scf_int + 1) : scf_int;

		//endi = ceil((endo+1)*((double)(ihinc))/(65536.0) + overlapH -1
		scf_int = (output_startpos + output_span) * h_ratio / ARSR2P_INC_FACTOR;
		scf_rem = (output_startpos + output_span) * h_ratio % ARSR2P_INC_FACTOR;
		scf_in_end = (scf_rem > 0) ? (scf_int + 1) : scf_int;

		if ((first_block == 1) && (last_block == 1)) { //the layer is included only in this block
			scf_read_start = 0;
			scf_read_end = DSS_WIDTH(src_rect.w);
			h_v_layer->block_info.last_tile = 1;
		} else if (first_block == 1) {  //first block of this layer
			scf_read_start = 0;
			scf_read_end = scf_in_end + ARSR2P_OVERLAPH - 1;
		} else {
			scf_read_start = 0;
			if (scf_in_start > ARSR2P_OVERLAPH)
				scf_read_start = scf_in_start - ARSR2P_OVERLAPH;

			if (last_block == 1) { //last block of this layer
				scf_read_end = DSS_WIDTH(src_rect.w);
				h_v_layer->block_info.last_tile = 1;
			} else {  //middle block of this layer
				scf_read_end = scf_in_end + ARSR2P_OVERLAPH - 1;
			}
		}

		if (scf_read_end > DSS_WIDTH(src_rect.w))  //lint !e574
			scf_read_end = DSS_WIDTH(src_rect.w);

		input_startpos = scf_read_start;
		input_span = scf_read_end - scf_read_start + 1;
		h_v_layer->block_info.h_ratio_arsr2p = h_ratio;
		h_v_layer->block_info.arsr2p_src_x = h_layer->src_rect.x;//new added
		h_v_layer->block_info.arsr2p_src_y = h_layer->src_rect.y;//new added
		h_v_layer->block_info.arsr2p_dst_x = h_layer->dst_rect.x;//new added
		h_v_layer->block_info.arsr2p_dst_y = h_layer->dst_rect.y;//new added
		h_v_layer->block_info.arsr2p_dst_w = h_layer->dst_rect.w;//new added

		// handle ROT
		rect_transform.x = h_layer->src_rect.x + input_startpos;
		rect_transform.y = h_layer->src_rect.y;
		rect_transform.w = input_span;
		rect_transform.h = h_layer->src_rect.h;
		h_v_layer->block_info.arsr2p_in_rect = rect_transform; //new added
		h_v_layer->src_rect = rect_transform;  //arsr2p input rect
		rect_across_rect(h_v_layer->src_rect, h_v_layer->src_rect_mask, &h_v_layer->src_rect_mask);
		h_v_layer->dst_rect = dst_cross_rect;	//arsr2p output rect
	}

	//scaling not in rchn v0 or scaling down in rchn v0
	if (((h_layer->src_rect.w != h_layer->dst_rect.w) && (h_layer->chn_idx != chn_idx_temp))
        || ((h_layer->src_rect.w > h_layer->dst_rect.w) && (h_layer->chn_idx == chn_idx_temp)))
	{
		/* check if arsr2p input has already extened width */
		if (h_v_layer->block_info.h_ratio_arsr2p) {
			//h_v_layer->block_info.arsr2p_in_rect = rect_transform; //new added
			dst_rect = rect_transform;
			h_v_layer->block_info.both_vscfh_arsr2p_used = 1;

			output_startpos = input_startpos;//new added
			//output_startpos = dst_rect.x - h_layer->src_rect.x;//new added
			output_span = dst_rect.w;
			input_startpos = output_startpos;
			input_span = output_span;
		} else {
			dst_rect = h_layer->dst_rect;
		}

		h_ratio = (DSS_WIDTH(h_layer->src_rect.w) * SCF_INC_FACTOR + SCF_INC_FACTOR / 2 - acc_hscl) /
			DSS_WIDTH(h_layer->dst_rect.w);//lint !e573

		scf_in_start = output_startpos * h_ratio / SCF_INC_FACTOR;
		scf_in_end = DSS_WIDTH(output_startpos + output_span) * h_ratio / SCF_INC_FACTOR;

		if ((first_block == 1) && (last_block == 1)) { //the layer is included only in this block
			acc_hscl = 0;
			scf_read_start = 0;
			scf_read_end = DSS_WIDTH(h_layer->src_rect.w);
		} else if (first_block == 1) {  //first block of this layer
			acc_hscl = 0;
			scf_read_start = 0;
			scf_read_end = scf_in_end + SCF_INPUT_OV;
		} else {
			scf_read_start = 0;
			if (scf_in_start > SCF_INPUT_OV)
				scf_read_start = scf_in_start - SCF_INPUT_OV;
			acc_hscl = output_startpos * h_ratio - scf_read_start * SCF_INC_FACTOR;

			if (last_block == 1) { //last block of this layer
				scf_read_end = DSS_WIDTH(h_layer->src_rect.w);
			} else {  //middle block of this layer
				scf_read_end = scf_in_end + SCF_INPUT_OV;
			}
		}

		if (scf_read_end > DSS_WIDTH(h_layer->src_rect.w)) //lint !e574
			scf_read_end = DSS_WIDTH(h_layer->src_rect.w);

		input_startpos = scf_read_start;
		input_span = scf_read_end - scf_read_start + 1;
		h_v_layer->block_info.h_ratio = h_ratio;
		h_v_layer->block_info.acc_hscl = acc_hscl;

		if (g_debug_ovl_block_composer) {
			HISI_FB_INFO("first_block=%d, last_block=%d, output_startpos=%d, output_span=%d, "
				"h_ratio=%d, acc_hscl=%d, scf_read_start=%d, scf_read_end=%d, input_startpos=%d, input_span=%d\n",
				first_block, last_block, output_startpos, output_span,
				h_ratio, acc_hscl, scf_read_start, scf_read_end, input_startpos, input_span);
		}
	}

	// handle ROT
	switch (h_v_layer->transform) {
	case HISI_FB_TRANSFORM_NOP:
	case HISI_FB_TRANSFORM_FLIP_V:
		rect_transform.x = h_layer->src_rect.x + input_startpos;
		rect_transform.y = h_layer->src_rect.y;
		rect_transform.w = input_span;
		rect_transform.h = h_layer->src_rect.h;
		break;
	case HISI_FB_TRANSFORM_ROT_180:
	case HISI_FB_TRANSFORM_FLIP_H:
		rect_transform.x = h_layer->src_rect.x + h_layer->src_rect.w - input_startpos - input_span;
		rect_transform.y = h_layer->src_rect.y;
		rect_transform.w = input_span;
		rect_transform.h = h_layer->src_rect.h;
		break;
	default:
		HISI_FB_ERR("unknow h_v_layer->transform=%d!\n", h_v_layer->transform);
		return -EINVAL;
	}

	h_v_layer->src_rect = rect_transform;
	rect_across_rect(h_v_layer->src_rect, h_v_layer->src_rect_mask, &h_v_layer->src_rect_mask);
	h_v_layer->dst_rect = dst_cross_rect;

	return 0;
}

static int wb_create_h_v_block_layer(dss_overlay_t *pov_req_h_v, dss_layer_t *h_layer, dss_layer_t *h_v_layer,
	dss_rect_t dst_cross_rect, dss_rect_t ov_block_rect)
{
	int input_startpos;
	int input_span;
	uint32_t output_startpos; //relative to overlay plane
	uint32_t output_span;
	uint32_t h_ratio = 0;
	uint32_t acc_hscl = 0;
	uint32_t scf_read_start = 0;
	uint32_t scf_read_end = 0;
	dss_rect_t rect_transform = {0};
	int first_block; //is the first block of this layer
	int last_block; //is the last block of this layer
	int scf_in_start = 0;
	int scf_in_end = 0;

	if ((NULL == h_layer) || (NULL == h_v_layer)) {
		HISI_FB_ERR("NULL ptr.\n");
		return -EINVAL;
	}

	first_block = (h_layer->dst_rect.x >= ov_block_rect.x) ? 1 : 0;
	last_block = ((ov_block_rect.x + ov_block_rect.w) >= (h_layer->dst_rect.x + h_layer->dst_rect.w)) ? 1 : 0;

	output_startpos = dst_cross_rect.x - h_layer->dst_rect.x;
	output_span = dst_cross_rect.w;
	input_startpos = output_startpos;
	input_span = output_span;

	if (h_layer->src_rect.w != h_layer->dst_rect.w) {
		h_ratio = (DSS_WIDTH(h_layer->src_rect.w) * SCF_INC_FACTOR + SCF_INC_FACTOR / 2 - acc_hscl) /
			DSS_WIDTH(h_layer->dst_rect.w);//lint !e573

		scf_in_start = output_startpos * h_ratio / SCF_INC_FACTOR;
		scf_in_end = DSS_WIDTH(output_startpos + output_span) * h_ratio / SCF_INC_FACTOR;

		if ((first_block == 1) && (last_block == 1)) { //the layer is included only in this block
			acc_hscl = 0;
			scf_read_start = 0;
			scf_read_end = DSS_WIDTH(h_layer->src_rect.w);
		} else if (first_block == 1) {  //first block of this layer
			acc_hscl = 0;
			scf_read_start = 0;
			scf_read_end = scf_in_end + SCF_INPUT_OV;
		} else {
			scf_read_start = 0;
			if (scf_in_start > SCF_INPUT_OV)
				scf_read_start = scf_in_start - SCF_INPUT_OV;
			acc_hscl = output_startpos * h_ratio - scf_read_start * SCF_INC_FACTOR;

			if (last_block == 1) { //last block of this layer
				scf_read_end = DSS_WIDTH(h_layer->src_rect.w);
			} else {  //middle block of this layer
				scf_read_end = scf_in_end + SCF_INPUT_OV;
			}
		}

		if (scf_read_end > DSS_WIDTH(h_layer->src_rect.w)) { //lint !e574
			scf_read_end = DSS_WIDTH(h_layer->src_rect.w);
		}

		input_startpos = scf_read_start;
		input_span = scf_read_end - scf_read_start + 1;

		pov_req_h_v->wb_layer_infos[0].wb_block_info.h_ratio = h_ratio;
		pov_req_h_v->wb_layer_infos[0].wb_block_info.acc_hscl = acc_hscl;

		if (g_debug_ovl_block_composer) {
			HISI_FB_INFO("first_block=%d, last_block=%d, output_startpos=%d, output_span=%d, "
				"h_ratio=%d, acc_hscl=%d, scf_read_start=%d, scf_read_end=%d, input_startpos=%d, input_span=%d\n",
				first_block, last_block, output_startpos, output_span,
				h_ratio, acc_hscl, scf_read_start, scf_read_end, input_startpos, input_span);
		}
	}

	// handle ROT
	switch (h_v_layer->transform) {
	case HISI_FB_TRANSFORM_NOP:
	case HISI_FB_TRANSFORM_FLIP_V:
		rect_transform.x = h_layer->src_rect.x + input_startpos;
		rect_transform.y = h_layer->src_rect.y;
		rect_transform.w = input_span;
		rect_transform.h = h_layer->src_rect.h;
		break;
	case HISI_FB_TRANSFORM_ROT_180:
	case HISI_FB_TRANSFORM_FLIP_H:
		rect_transform.x = h_layer->src_rect.x + h_layer->src_rect.w - input_startpos - input_span;
		rect_transform.y = h_layer->src_rect.y;
		rect_transform.w = input_span;
		rect_transform.h = h_layer->src_rect.h;
		break;
	default:
		HISI_FB_ERR("unknow h_v_layer->transform=%d!\n", h_v_layer->transform);
		return -EINVAL;
	}

	h_v_layer->src_rect = rect_transform;
	rect_across_rect(h_v_layer->src_rect, h_v_layer->src_rect_mask, &h_v_layer->src_rect_mask);

	h_v_layer->dst_rect = dst_cross_rect;

	pov_req_h_v->wb_layer_infos[0].wb_block_info.src_rect.x = pov_req_h_v->wb_layer_infos[0].src_rect.x + input_startpos;
	pov_req_h_v->wb_layer_infos[0].wb_block_info.src_rect.y = pov_req_h_v->wb_layer_infos[0].src_rect.y;
	pov_req_h_v->wb_layer_infos[0].wb_block_info.src_rect.w = input_span;
	pov_req_h_v->wb_layer_infos[0].wb_block_info.src_rect.h = pov_req_h_v->wb_layer_infos[0].src_rect.h;

	pov_req_h_v->wb_layer_infos[0].wb_block_info.dst_rect = dst_cross_rect;

	if (g_debug_ovl_block_composer) {
		HISI_FB_INFO("layer:src_rect[%d:%d:%d:%d], dst_rect[%d:%d:%d:%d]!\n"
			"wb_layer:src_rect[%d:%d:%d:%d], dst_rect[%d:%d:%d:%d]!\n",
			h_v_layer->src_rect.x, h_v_layer->src_rect.y, h_v_layer->src_rect.w, h_v_layer->src_rect.h,
			h_v_layer->dst_rect.x, h_v_layer->dst_rect.y, h_v_layer->dst_rect.w, h_v_layer->dst_rect.h,
			pov_req_h_v->wb_layer_infos[0].wb_block_info.src_rect.x, pov_req_h_v->wb_layer_infos[0].wb_block_info.src_rect.y,
			pov_req_h_v->wb_layer_infos[0].wb_block_info.src_rect.w, pov_req_h_v->wb_layer_infos[0].wb_block_info.src_rect.h,
			pov_req_h_v->wb_layer_infos[0].wb_block_info.dst_rect.x, pov_req_h_v->wb_layer_infos[0].wb_block_info.dst_rect.y,
			pov_req_h_v->wb_layer_infos[0].wb_block_info.dst_rect.w, pov_req_h_v->wb_layer_infos[0].wb_block_info.dst_rect.h);
	}

	return 0;
}
/*lint +e713 +e732 +e737 +e834 +e838 +e845*/
int get_block_layers(dss_overlay_t *pov_req, dss_overlay_block_t *pov_h_block,
	dss_rect_t ov_block_rect, dss_overlay_t *pov_req_h_v)
{
	uint32_t i = 0;
	int ret = 0;
	dss_rect_t dst_cross_rect;
	dss_rect_t wb_ov_rect;
	dss_overlay_block_t *pov_h_v_block = NULL;
	dss_layer_t *h_layer = NULL;
	dss_layer_t *h_v_layer = NULL;
	int h_v_layer_idx = 0;

	if (NULL == pov_req) {
		HISI_FB_ERR("pov_req is NULL");
		return -EINVAL;
	}
	if (NULL == pov_h_block) {
		HISI_FB_ERR("pov_h_block is NULL");
		return -EINVAL;
	}
	if (NULL == pov_req_h_v) {
		HISI_FB_ERR("pov_req_h_v is NULL");
		return -EINVAL;
	}

	if (!ov_block_rect.w || !ov_block_rect.h) {
		HISI_FB_ERR("invaild args, ov_block_rect(%d,%d,%d,%d)!\n",
			ov_block_rect.x, ov_block_rect.y, ov_block_rect.w, ov_block_rect.y);
		return -1;
	}

	// init pov_req_v_block
	pov_h_v_block = (dss_overlay_block_t *)pov_req_h_v->ov_block_infos_ptr;
	memcpy(pov_req_h_v, pov_req, sizeof(dss_overlay_t));
	pov_req_h_v->ov_block_infos_ptr = (uint64_t)(pov_h_v_block);

	if (calc_dest_block_size(pov_req, pov_h_block) == BLOCK_SIZE_INVALID) {
		pov_req_h_v->ov_block_nums = 1;
		memcpy(pov_h_v_block, pov_h_block, sizeof(dss_overlay_block_t));
		return 0;
	}

	pov_h_v_block->layer_nums = 0;
	h_v_layer_idx = 0;
	memcpy(&pov_h_v_block->ov_block_rect, &pov_h_block->ov_block_rect, sizeof(dss_rect_t));
	wb_ov_rect.x = pov_req->wb_ov_rect.x + ov_block_rect.x;
	wb_ov_rect.y = pov_req->wb_ov_rect.y;
	wb_ov_rect.w = ov_block_rect.w;
	wb_ov_rect.h = ov_block_rect.h;

	for (i = 0; i < pov_h_block->layer_nums; i++) {
		h_layer = &(pov_h_block->layer_infos[i]);

		ret = rect_across_rect(h_layer->dst_rect, wb_ov_rect, &dst_cross_rect);
		if (ret == 0) {
			continue;
		}

		h_v_layer = &(pov_h_v_block->layer_infos[h_v_layer_idx]);
		memcpy(h_v_layer, h_layer, sizeof(dss_layer_t));
		h_v_layer->layer_idx = h_v_layer_idx;

		if (pov_req->wb_compose_type == DSS_WB_COMPOSE_MEDIACOMMON) {
			ret = wb_create_h_v_block_layer(pov_req_h_v, h_layer, h_v_layer, dst_cross_rect, wb_ov_rect);
		} else {
			ret = create_h_v_block_layer(h_layer, h_v_layer, dst_cross_rect, wb_ov_rect);
		}

		if ((ret != 0) || g_debug_ovl_block_composer) {
			HISI_FB_INFO("h_layer[%d](transform[%d], wb_ov_rect[%d,%d,%d,%d], src_rect[%d,%d,%d,%d], dst_rect[%d,%d,%d,%d]), "
				"h_v_layer[%d](transform[%d], src_rect[%d,%d,%d,%d], dst_rect[%d,%d,%d,%d], dst_cross_rect[%d,%d,%d,%d]),"
				"wb_block_info.h_ratio=%d, wb_block_info.acc_hscl=%d!\n",
				i, h_layer->transform, wb_ov_rect.x, wb_ov_rect.y, wb_ov_rect.w, wb_ov_rect.h,
				h_layer->src_rect.x, h_layer->src_rect.y, h_layer->src_rect.w, h_layer->src_rect.h,
				h_layer->dst_rect.x, h_layer->dst_rect.y, h_layer->dst_rect.w, h_layer->dst_rect.h,
				h_v_layer_idx, h_v_layer->transform,
				h_v_layer->src_rect.x, h_v_layer->src_rect.y, h_v_layer->src_rect.w, h_v_layer->src_rect.h,
				h_v_layer->dst_rect.x, h_v_layer->dst_rect.y, h_v_layer->dst_rect.w, h_v_layer->dst_rect.h,
				dst_cross_rect.x, dst_cross_rect.y, dst_cross_rect.w, dst_cross_rect.h,
				pov_req_h_v->wb_layer_infos[0].wb_block_info.h_ratio,
				pov_req_h_v->wb_layer_infos[0].wb_block_info.acc_hscl);
		}

		if (ret != 0) {
			HISI_FB_ERR("create_h_v_block_layer failed, h_layer[%d], h_v_layer[%d]!\n",
				i, h_v_layer_idx);
			break;
		}

		h_v_layer_idx++;
		pov_h_v_block->layer_nums = h_v_layer_idx;
	}

	return ret;
}

int get_wb_layer_block_rect(dss_wb_layer_t *wb_layer, bool has_wb_scl, dss_rect_t *wb_layer_block_rect)
{
	if (wb_layer == NULL) {
		HISI_FB_ERR("wb_layer is NULL point!\n");
		return -1;
	}

	if (wb_layer_block_rect == NULL) {
		HISI_FB_ERR("wb_layer_block_rect is NULL point!\n");
		return -1;
	}

	if (has_wb_scl) {
		if (wb_layer->transform == (HISI_FB_TRANSFORM_ROT_90 | HISI_FB_TRANSFORM_FLIP_V)) {//lint !e655
			wb_layer_block_rect->x = wb_layer->dst_rect.x;
			wb_layer_block_rect->y = wb_layer->dst_rect.y;
			wb_layer_block_rect->w = wb_layer->dst_rect.h;
			wb_layer_block_rect->h = wb_layer->dst_rect.w;
		} else {
			*wb_layer_block_rect = wb_layer->dst_rect;
		}
	} else {
		*wb_layer_block_rect = wb_layer->src_rect;
	}

	return 0;
}

