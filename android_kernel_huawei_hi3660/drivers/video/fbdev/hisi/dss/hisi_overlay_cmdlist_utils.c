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


#define MAX_ITEM_OFFSET	(0x3F)
#define CMDLIST_ADDR_OFFSET	(0x3FFFF)

#define CMDLIST_HEADER_LEN	(SZ_1K)
#define CMDLIST_ITEM_LEN	(SZ_8K)
#define MAX_ITEM_INDEX	(SZ_1K)

dss_cmdlist_data_t *g_cmdlist_data = NULL;
uint32_t g_online_cmdlist_idxs = 0;
uint32_t g_offline_cmdlist_idxs = 0;

/* get cmdlist indexs */
int hisi_cmdlist_get_cmdlist_idxs(dss_overlay_t *pov_req,
	uint32_t *cmdlist_pre_idxs, uint32_t *cmdlist_idxs)
{
	uint32_t cmdlist_idxs_temp = 0;
	int i = 0;
	int k = 0;
	int m = 0;
	dss_layer_t *layer = NULL;
	dss_wb_layer_t *wb_layer = NULL;
	dss_overlay_block_t *pov_h_block_infos = NULL;
	dss_overlay_block_t *pov_h_block = NULL;
	bool no_ovl_idx = false;

	if (NULL == pov_req) {
		HISI_FB_ERR("pov_req is NULL");
		return -EINVAL;
	}

	pov_h_block_infos = (dss_overlay_block_t *)pov_req->ov_block_infos_ptr;
	for (m = 0; m < pov_req->ov_block_nums; m++) {
		pov_h_block = &(pov_h_block_infos[m]);
		for (i = 0; i < pov_h_block->layer_nums; i++) {
			layer = &(pov_h_block->layer_infos[i]);

			if (layer->need_cap & (CAP_BASE | CAP_DIM | CAP_PURE_COLOR))
				continue;

			if (layer->chn_idx == DSS_RCHN_V2) {
				cmdlist_idxs_temp |= (1 << DSS_CMDLIST_V2);
			} else {
				cmdlist_idxs_temp |= (1 << layer->chn_idx);
			}
		}
	}

	if (pov_req->wb_enable == 1) {
		for (k = 0; k < pov_req->wb_layer_nums; k++) {
			wb_layer = &(pov_req->wb_layer_infos[k]);

			if (wb_layer->chn_idx == DSS_WCHN_W2) {
				no_ovl_idx = true;
				cmdlist_idxs_temp |= (1 << DSS_CMDLIST_W2);
			} else {
				cmdlist_idxs_temp |= (1 << wb_layer->chn_idx);
			}
		}
	}

	if (no_ovl_idx == false) {
		cmdlist_idxs_temp |= (1 << (DSS_CMDLIST_OV0 + pov_req->ovl_idx));
	}

	if (cmdlist_idxs_temp & (~HISI_DSS_CMDLIST_IDXS_MAX)) {
		HISI_FB_ERR("cmdlist_idxs_temp(0x%x) is invalid!\n", cmdlist_idxs_temp);
		return -EINVAL;
	}

	if (cmdlist_idxs && cmdlist_pre_idxs) {
		*cmdlist_idxs = cmdlist_idxs_temp;
		*cmdlist_pre_idxs &= (~ (*cmdlist_idxs));
	} else if (cmdlist_idxs) {
		*cmdlist_idxs = cmdlist_idxs_temp;
	} else if (cmdlist_pre_idxs) {
		*cmdlist_pre_idxs = cmdlist_idxs_temp;
	} else {
		HISI_FB_ERR("cmdlist_idxs && cmdlist_pre_idxs is NULL!\n");
		return -EINVAL;
	}

	if (g_debug_ovl_cmdlist) {
		HISI_FB_INFO("cmdlist_pre_idxs(0x%x), cmdlist_idxs(0x%x).\n",
			(cmdlist_pre_idxs ? *cmdlist_pre_idxs : 0),
			(cmdlist_idxs ? *cmdlist_idxs : 0));
	}

	return 0;
}

uint32_t hisi_cmdlist_get_cmdlist_need_start(struct hisi_fb_data_type *hisifd, uint32_t cmdlist_idxs)
{
	uint32_t cmdlist_idxs_temp = 0;

	if (NULL == hisifd) {
		HISI_FB_ERR("hisifd is NULL");
		return 0;
	}

	cmdlist_idxs_temp = g_offline_cmdlist_idxs;
	g_offline_cmdlist_idxs |= cmdlist_idxs;
	cmdlist_idxs_temp = (g_offline_cmdlist_idxs & (~cmdlist_idxs_temp));

	cmdlist_idxs_temp |= (cmdlist_idxs & g_online_cmdlist_idxs);
	g_online_cmdlist_idxs &= (~cmdlist_idxs_temp);

	if (g_debug_ovl_cmdlist) {
		HISI_FB_INFO("g_online_cmdlist_idxs=0x%x, cmdlist_idxs_need_start=0x%x\n",
			g_online_cmdlist_idxs, cmdlist_idxs_temp);
	}

	return cmdlist_idxs_temp;
}

/*
** data0: addr0[17:0]
** data1: addr0[17:0] + addr1[5:0]
** data2: addr0[17:0] + addr2[5:0]
**
** cnt[1:0]:
** 2'b00:	reg0
** 2'b01: reg0, reg1
** 2'b10: reg0, reg1, reg2
** 2'b11: ((inp32(addr0) & data1) | data2) -> addr0
*/
bool hisi_check_cmdlist_paremeters_validate(uint8_t bw, uint8_t bs)
{
	if((bs > 32)||(bw > 32)||((bw + bs) > 32)){
		HISI_FB_ERR("Can't do this,which may cause overflow.");
		return false;
	}
	return true;
}
void hisi_cmdlist_set_reg(struct hisi_fb_data_type *hisifd, char __iomem *addr,
	uint32_t value, uint8_t bw, uint8_t bs)
{
	if (!hisi_check_cmdlist_paremeters_validate(bw,bs)) {
		return;
	}
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeclaration-after-statement"
	uint64_t mask = ((uint64_t)1 << bw) - 1;//lint !e647
#pragma GCC diagnostic pop
	dss_cmdlist_node_t *node = NULL;
	int cmdlist_idx = -1;
	int index = 0;
	uint32_t new_addr = 0;
	uint32_t old_addr = 0;
	uint64_t temp_addr = 0;
	int condition = 0;

	if (NULL == addr) {
		HISI_FB_ERR("addr is NULL");
		return;
	}
	if (NULL == hisifd) {
		HISI_FB_ERR("hisifd is NULL");
		return;
	}

	cmdlist_idx = hisifd->cmdlist_idx;
	if ((cmdlist_idx < 0) || (cmdlist_idx >= HISI_DSS_CMDLIST_MAX)) {
		HISI_FB_ERR("cmdlist_idx is invalid");
		return;
	}

	node = list_entry(hisifd->cmdlist_data->cmdlist_head_temp[cmdlist_idx].prev, dss_cmdlist_node_t, list_node);
	if (NULL == node) {
		HISI_FB_ERR("node is NULL");
		return;
	}

	if (node->node_type == CMDLIST_NODE_NOP) {
		HISI_FB_ERR("can't set register value to NOP node!");
		return;
	}

	index = node->item_index;
	if (hisifd->index == MEDIACOMMON_PANEL_IDX) {
		temp_addr = (uint64_t)(addr);
		new_addr = (uint32_t)(temp_addr & 0xFFFFFFFF);
	} else {
		new_addr = (uint32_t)(addr - hisifd->dss_base + hisifd->dss_base_phy);
	}

	new_addr = (new_addr >> 2) & CMDLIST_ADDR_OFFSET;
	old_addr = node->list_item[index].reg_addr.ul32 & CMDLIST_ADDR_OFFSET;
	condition = (((new_addr - old_addr) < MAX_ITEM_OFFSET) && (new_addr >= old_addr));

	if (bw != 32) {
		if (node->item_flag != 0)
			index++;

		node->list_item[index].reg_addr.bits.add0 = new_addr;
		node->list_item[index].data0 = value;
		node->list_item[index].data1 = (uint32_t) ((~(mask << bs)) & 0xFFFFFFFF);
		node->list_item[index].data2 = (uint32_t) (((mask & value) << bs) & 0xFFFFFFFF);
		node->list_item[index].reg_addr.bits.cnt = 3;
		node->item_flag = 3;
	} else {
		if (node->item_flag == 0) {
			node->list_item[index].reg_addr.bits.add0 = new_addr;
			node->list_item[index].data0 = value;
			node->list_item[index].reg_addr.bits.cnt = 0;
			node->item_flag = 1;
		} else if (node->item_flag == 1 && condition) {
			node->list_item[index].reg_addr.bits.add1 = new_addr - old_addr;
			node->list_item[index].data1 = value;
			node->list_item[index].reg_addr.bits.cnt = 1;
			node->item_flag = 2;
		} else if (node->item_flag == 2 && condition) {
			node->list_item[index].reg_addr.bits.add2 = new_addr - old_addr;
			node->list_item[index].data2 = value;
			node->list_item[index].reg_addr.bits.cnt = 2;
			node->item_flag = 3;
		} else {
			index++;
			node->list_item[index].reg_addr.bits.add0 = new_addr;
			node->list_item[index].data0 = value;
			node->list_item[index].reg_addr.bits.cnt = 0;
			node->item_flag = 1;
		}
	}

	if (index >= MAX_ITEM_INDEX) {
		HISI_FB_ERR("index=%d is too large(1k)!\n", index);
		return;
	}

	node->item_index = index;
	node->list_header->total_items.bits.count = node->item_index + 1;
}

/*
** flush cache for cmdlist, make sure that
** cmdlist has writen through to memory before config register
*/
void hisi_cmdlist_flush_cache(struct hisi_fb_data_type *hisifd, uint32_t cmdlist_idxs)
{
	uint32_t i = 0;
	uint32_t cmdlist_idxs_temp = 0;
	dss_cmdlist_node_t *node = NULL;
	dss_cmdlist_node_t *_node_ = NULL;
	struct list_head *cmdlist_heads = NULL;

	if (!hisifd) {
		HISI_FB_ERR("hisifd is NULL!\n");
		return ;
	}

	cmdlist_idxs_temp = cmdlist_idxs;

	for (i = 0; i < HISI_DSS_CMDLIST_MAX; i++) {
		if ((cmdlist_idxs_temp & 0x1) == 0x1) {
			cmdlist_heads = &(hisifd->cmdlist_data->cmdlist_head_temp[i]);
			if (!cmdlist_heads) {
				HISI_FB_ERR("cmdlist_data is NULL!\n");
				continue;
			}

			list_for_each_entry_safe_reverse(node, _node_, cmdlist_heads, list_node) {
				if (node) {
					dma_sync_single_for_device(NULL, node->header_phys,
						node->header_len, DMA_TO_DEVICE);
					dma_sync_single_for_device(NULL, node->item_phys,
						node->item_len, DMA_TO_DEVICE);
				}
			}
		}

		cmdlist_idxs_temp = cmdlist_idxs_temp >> 1;
	}
}

dss_cmdlist_node_t* hisi_cmdlist_node_alloc(struct hisi_fb_data_type *hisifd)
{
	dss_cmdlist_node_t *node = NULL;

	if (!hisifd) {
		HISI_FB_ERR("hisifd is NULL!\n");
		return NULL;
	}

	if (!(hisifd->cmdlist_pool)) {
		HISI_FB_ERR("fb%d, cmdlist_pool is NULL!\n", hisifd->index);
		return NULL;
	}

	node = (dss_cmdlist_node_t *)kzalloc(sizeof(dss_cmdlist_node_t), GFP_KERNEL);
	if (IS_ERR(node)) {
		HISI_FB_ERR("failed to alloc dss_cmdlist_node_t!");
		return NULL;
	}

	memset(node, 0, sizeof(dss_cmdlist_node_t));

	node->header_len = roundup(CMDLIST_HEADER_LEN, PAGE_SIZE);
	node->item_len = roundup(CMDLIST_ITEM_LEN, PAGE_SIZE);

	/* alloc buffer for header */
	node->list_header = (cmd_header_t *)gen_pool_alloc(hisifd->cmdlist_pool, node->header_len);
	if (!node->list_header) {
		HISI_FB_ERR("fb%d, header gen_pool_alloc failed!\n", hisifd->index);
		goto err_header_alloc;
	}

	node->header_phys = gen_pool_virt_to_phys(hisifd->cmdlist_pool, (unsigned long)node->list_header);

	/* alloc buffer for items */
	node->list_item = (cmd_item_t *)gen_pool_alloc(hisifd->cmdlist_pool, node->item_len);
	if (!node->list_item) {
		HISI_FB_ERR("fb%d, item gen_pool_alloc failed!\n", hisifd->index);
		goto err_item_alloc;
	}

	node->item_phys = gen_pool_virt_to_phys(hisifd->cmdlist_pool, (unsigned long)node->list_item);

	memset(node->list_header, 0, node->header_len);
	memset(node->list_item, 0, node->item_len);

	/* fill node info */
	node->item_flag= 0;
	node->item_index= 0;

	node->is_used = 0;
	node->node_type = CMDLIST_NODE_NONE;
	return node;

err_item_alloc:
	if (node->list_header) {
		gen_pool_free(hisifd->cmdlist_pool, (unsigned long)node->list_header, node->header_len);
	}
err_header_alloc:
	if (node) {
		kfree(node);
		node = NULL;
	}

	return node;
}

void hisi_cmdlist_node_free(struct hisi_fb_data_type *hisifd, dss_cmdlist_node_t *node)
{
	if (!hisifd) {
		HISI_FB_ERR("hisifd is NULL!\n");
		return ;
	}

	if (!node) {
		HISI_FB_ERR("node is NULL!\n");
		return ;
	}

	if (hisifd->cmdlist_pool && node->list_header) {
		gen_pool_free(hisifd->cmdlist_pool, (unsigned long)node->list_header, node->header_len);
		node->list_header = NULL;
	}

	if (hisifd->cmdlist_pool && node->list_item) {
		gen_pool_free(hisifd->cmdlist_pool, (unsigned long)node->list_item, node->item_len);
		node->list_item = NULL;
	}

	kfree(node);
	node = NULL;
}

static dss_cmdlist_node_t* hisi_cmdlist_get_free_node(dss_cmdlist_node_t *node[], int *id)
{
	int i = 0;

	for (i = 0; i < HISI_DSS_CMDLIST_NODE_MAX; i++) {
		if (node[i] && (node[i]->is_used == 0)) {
			node[i]->is_used = 1;
			*id = i + 1;
			return node[i];
		}
	}

	return NULL;
}

static bool hisi_cmdlist_addr_check(struct hisi_fb_data_type *hisifd, uint32_t *list_addr)
{
	if (NULL == hisifd) {
		HISI_FB_ERR("hisifd is NULL");
		return false;
	}

	if ((*list_addr > (uint32_t)(hisifd->cmdlist_pool_phy_addr + hisifd->sum_cmdlist_pool_size))
		||(*list_addr < (uint32_t)hisifd->cmdlist_pool_phy_addr)) {
		HISI_FB_ERR("fb%d, cmdlist_addr is invalid, sum_cmdlist_pool_size=%zu.\n", hisifd->index, hisifd->sum_cmdlist_pool_size);
		*list_addr = hisifd->cmdlist_pool_phy_addr;
		return false;
	}

	return true;
}

int hisi_cmdlist_add_nop_node(struct hisi_fb_data_type *hisifd, uint32_t cmdlist_idxs, int pending, int reserved)
{
	dss_cmdlist_node_t *node = NULL;
	dss_cmdlist_node_t *first_node = NULL;
	uint32_t cmdlist_idxs_temp = 0;
	int i = 0;
	int id = 0;

	if (NULL == hisifd) {
		HISI_FB_ERR("hisifd is NULL");
		return -EINVAL;
	}

	cmdlist_idxs_temp = cmdlist_idxs;

	for (i = 0; i < HISI_DSS_CMDLIST_MAX; i++) {
		if ((cmdlist_idxs_temp & 0x1) == 0x1) {
			node = hisi_cmdlist_get_free_node(hisifd->cmdlist_data->cmdlist_nodes_temp[i], &id);
			if (!node) {
				HISI_FB_ERR("failed to hisi_get_free_cmdlist_node!\n");
				return -EINVAL;
			}

			node->list_header->flag.bits.id = id;
			node->list_header->flag.bits.nop = 0x1;
			node->list_header->flag.bits.pending = pending ? 0x1 : 0x0;
			node->list_header->flag.bits.valid_flag = CMDLIST_NODE_VALID;
			node->list_header->flag.bits.last = 0;
			node->list_header->next_list = node->header_phys;
			hisi_cmdlist_addr_check(hisifd, &(node->list_header->next_list));

			node->is_used = 1;
			node->node_type = CMDLIST_NODE_NOP;
			node->reserved = reserved ? 0x1 : 0x0;

			/*add this nop to list*/
			list_add_tail(&(node->list_node), &(hisifd->cmdlist_data->cmdlist_head_temp[i]));

			if (node->list_node.prev != &(hisifd->cmdlist_data->cmdlist_head_temp[i])) {
				dss_cmdlist_node_t *pre_node = NULL;
				first_node = list_first_entry(&(hisifd->cmdlist_data->cmdlist_head_temp[i]), dss_cmdlist_node_t, list_node);
				pre_node = list_entry(node->list_node.prev, dss_cmdlist_node_t, list_node);
				pre_node->list_header->next_list = first_node->header_phys;
				hisi_cmdlist_addr_check(hisifd, &(pre_node->list_header->next_list));

				if (node->list_header->flag.bits.pending == 0x1) {
					pre_node->reserved = 0x0;
				}

				pre_node->list_header->flag.bits.task_end = 0x1;
				node->list_header->flag.bits.task_end = 0x1;

				if (g_debug_ovl_cmdlist) {
					HISI_FB_DEBUG("i = %d, next_list = 0x%x\n", i, (uint32_t)(node->header_phys));
				}
			}
		}

		cmdlist_idxs_temp = cmdlist_idxs_temp >> 1;
	}

	return 0;
}

int hisi_cmdlist_add_new_node(struct hisi_fb_data_type *hisifd,
	uint32_t cmdlist_idxs, int pending, int task_end, int remove, int last, uint32_t wb_type)
{
	dss_cmdlist_node_t *node = NULL;
	uint32_t cmdlist_idxs_temp = 0;
	int i = 0;
	int id = 0;

	if (NULL == hisifd) {
		HISI_FB_ERR("hisifd is NULL");
		return -EINVAL;
	}

	cmdlist_idxs_temp = cmdlist_idxs;

	for (i = 0; i < HISI_DSS_CMDLIST_MAX; i++) {
		if ((cmdlist_idxs_temp & 0x1) == 0x1) {
			node = hisi_cmdlist_get_free_node(hisifd->cmdlist_data->cmdlist_nodes_temp[i], &id);
			if (!node) {
				HISI_FB_ERR("failed to hisi_get_free_cmdnode!\n");
				return -EINVAL;
			}

			/*fill the header and item info*/
			node->list_header->flag.bits.id = id;
			node->list_header->flag.bits.pending = pending ? 0x1 : 0x0;

			if (i < DSS_CMDLIST_W0) {
				node->list_header->flag.bits.event_list = remove ? 0x8 : (0xE + i);
			} else if (i < DSS_CMDLIST_OV0) {
				node->list_header->flag.bits.event_list = remove ? 0x8 : (0x16 + i);
			} else if (i == DSS_CMDLIST_V2) {
				node->list_header->flag.bits.event_list = remove ? 0x8 : 0x16;
			} else if (i == DSS_CMDLIST_W2) {
				node->list_header->flag.bits.event_list = remove ? 0x8 : 0x20;
			} else {
				node->list_header->flag.bits.event_list = remove ? 0x8 : (0xE + i);
			}

			node->list_header->flag.bits.task_end = task_end ? 0x1 : 0x0;
			node->list_header->flag.bits.last = last ? 0x1 : 0x0;

			node->list_header->flag.bits.valid_flag = CMDLIST_NODE_VALID;
			node->list_header->flag.bits.exec = 0x1;
			node->list_header->list_addr = node->item_phys;
			node->list_header->next_list = node->header_phys;

			hisi_cmdlist_addr_check(hisifd, &(node->list_header->list_addr));
			hisi_cmdlist_addr_check(hisifd, &(node->list_header->next_list));

			node->is_used = 1;
			node->node_type = CMDLIST_NODE_FRAME;
			node->item_flag = 0;
			node->reserved = 0;

			/* add this nop to list */
			list_add_tail(&(node->list_node), &(hisifd->cmdlist_data->cmdlist_head_temp[i]));

			if (node->list_node.prev != &(hisifd->cmdlist_data->cmdlist_head_temp[i])) {
				dss_cmdlist_node_t *pre_node = NULL;
				pre_node = list_entry(node->list_node.prev, dss_cmdlist_node_t, list_node);
				pre_node->list_header->next_list = node->header_phys;
				hisi_cmdlist_addr_check(hisifd, &(pre_node->list_header->next_list));
				pre_node->reserved = 0x0;
				if (g_debug_ovl_cmdlist) {
					HISI_FB_DEBUG("i = %d, next_list = 0x%x\n",  i, (uint32_t)node->header_phys);
				}
			}
		}

		cmdlist_idxs_temp  = cmdlist_idxs_temp >> 1;
	}

	return 0;
}

int hisi_cmdlist_del_all_node(struct list_head *cmdlist_heads)
{
	dss_cmdlist_node_t *node = NULL;
	dss_cmdlist_node_t *_node_ = NULL;

	if (NULL == cmdlist_heads) {
		HISI_FB_ERR("cmdlist_heads is NULL");
		return -EINVAL;
	}

	list_for_each_entry_safe(node, _node_, cmdlist_heads, list_node) {
		if (node->reserved != 0x1) {
			list_del(&node->list_node);

			node->list_header->flag.bits.exec = 0;
			node->list_header->flag.bits.last = 1;
			node->list_header->flag.bits.nop = 0;
			node->list_header->flag.bits.interrupt = 0;
			node->list_header->flag.bits.pending = 0;
			node->list_header->flag.bits.id = 0;
			node->list_header->flag.bits.event_list = 0;
			node->list_header->flag.bits.qos = 0;
			node->list_header->flag.bits.task_end = 0;
			node->list_header->flag.bits.valid_flag = 0;
			node->list_header->total_items.ul32 = 0;

			memset(node->list_item, 0, CMDLIST_ITEM_LEN);

			node->item_index = 0;
			node->item_flag = 0;
			node->node_type = CMDLIST_NODE_NONE;
			node->is_used = 0;
		}
	}

	return 0;
}

int hisi_cmdlist_check_cmdlist_state(struct hisi_fb_data_type *hisifd, uint32_t cmdlist_idxs)
{
	char __iomem *cmdlist_base = NULL;
	uint32_t offset = 0;
	uint32_t tmp = 0;
	uint32_t cmdlist_idxs_temp = 0;
	int i = 0;
	int delay_count = 0;
	bool is_timeout = true;
	int ret = 0;

	if (NULL == hisifd) {
		HISI_FB_ERR("hisifd is NULL");
		return -EINVAL;
	}

	if (hisifd->index == MEDIACOMMON_PANEL_IDX) {
		cmdlist_base = hisifd->media_common_base + DSS_CMDLIST_OFFSET;
	} else {
		cmdlist_base = hisifd->dss_base + DSS_CMDLIST_OFFSET;
	}

	offset = 0x40;
	cmdlist_idxs_temp = cmdlist_idxs;

	for (i = 0; i < HISI_DSS_CMDLIST_MAX; i++) {
		if ((cmdlist_idxs_temp & 0x1) == 0x1) {
			while (1) {
				tmp = inp32(cmdlist_base + CMDLIST_CH0_STATUS + i * offset);
				if (((tmp & 0xF) == 0x0) || delay_count > 5000) {
					is_timeout = (delay_count > 5000) ? true : false;
					delay_count = 0;
					break;
				} else {
					udelay(1);
					++delay_count;
				}
			}

			if (is_timeout) {
				HISI_FB_ERR("cmdlist_ch%d not in idle state,ints=0x%x !\n", i, tmp);
				ret = -1;
			}
		}

		cmdlist_idxs_temp = (cmdlist_idxs_temp >> 1);
	}

	return ret;
}

/*
** stop the pending state for one new frame
** if the current cmdlist status is e_status_wait.
*/
int hisi_cmdlist_exec(struct hisi_fb_data_type *hisifd, uint32_t cmdlist_idxs)
{
	char __iomem *cmdlist_base = NULL;
	uint32_t offset = 0;
	uint32_t tmp = 0;
	uint32_t cmdlist_idxs_temp = 0;
	int i = 0;
	int delay_count = 0;
	bool is_timeout = true;

	if (NULL == hisifd) {
		HISI_FB_ERR("hisifd is NULL");
		return -EINVAL;
	}

	if (hisifd->index == MEDIACOMMON_PANEL_IDX) {
		cmdlist_base = hisifd->media_common_base + DSS_CMDLIST_OFFSET;
	} else {
		cmdlist_base = hisifd->dss_base + DSS_CMDLIST_OFFSET;
	}
	offset = 0x40;
	cmdlist_idxs_temp = cmdlist_idxs;

	for (i = 0; i < HISI_DSS_CMDLIST_MAX; i++) {
		if ((cmdlist_idxs_temp & 0x1) == 0x1) {
			while (1) {
				tmp = inp32(cmdlist_base + CMDLIST_CH0_STATUS + i * offset);
				if (((tmp & 0xF) == 0x0) || delay_count > 500) {
					is_timeout = (delay_count > 500) ? true : false;
					delay_count = 0;
					break;
				} else {
					udelay(1);
					++delay_count;
				}
			}

			if (is_timeout) {
				HISI_FB_ERR("cmdlist_ch%d not in idle state,ints=0x%x !\n", i, tmp);

				if (g_debug_ovl_cmdlist) {
					hisi_cmdlist_dump_all_node(hisifd, NULL, cmdlist_idxs);
				}

				if (g_debug_ovl_offline_composer_hold) {
					mdelay(HISI_DSS_COMPOSER_HOLD_TIME);
				}
			}
		}

		cmdlist_idxs_temp = (cmdlist_idxs_temp >> 1);
	}


	return 0;
}

/*
**start cmdlist.
**it will set cmdlist into pending state.
*/
extern uint32_t g_dss_module_ovl_base[DSS_MCTL_IDX_MAX][MODULE_OVL_MAX];
int hisi_cmdlist_config_start(struct hisi_fb_data_type *hisifd, int mctl_idx, uint32_t cmdlist_idxs, uint32_t wb_compose_type)
{
	char __iomem *mctl_base = NULL;
	char __iomem *cmdlist_base = NULL;
	dss_cmdlist_node_t *cmdlist_node = NULL;
	uint32_t offset = 0;
	uint32_t list_addr = 0;
	uint32_t cmdlist_idxs_temp = 0;
	int temp = 0;
	int i = 0;
	int status_temp = 0;
	int ints_temp = 0;

	if (NULL == hisifd) {
		HISI_FB_ERR("hisifd is NULL");
		return -EINVAL;
	}

	if ((mctl_idx < 0) || (mctl_idx >= DSS_MCTL_IDX_MAX)) {
		HISI_FB_ERR("mctl_idx=%d is invalid.", mctl_idx);
		return -EINVAL;
	}

	mctl_base = hisifd->dss_base + g_dss_module_ovl_base[mctl_idx][MODULE_MCTL_BASE];
	cmdlist_base = hisifd->dss_base + DSS_CMDLIST_OFFSET;
	offset = 0x40;
	cmdlist_idxs_temp = cmdlist_idxs;

	if (g_dss_version_tag != FB_ACCEL_HI366x) {
		dsb(sy);
	}

	for (i = 0; i < HISI_DSS_CMDLIST_MAX; i++) {
		if ((cmdlist_idxs_temp & 0x1) == 0x1) {
			status_temp =  inp32(cmdlist_base + CMDLIST_CH0_STATUS + i * offset);
			ints_temp = inp32(cmdlist_base + CMDLIST_CH0_INTS + i * offset);

			if (mctl_idx >= DSS_MCTL2) {
				cmdlist_node = list_first_entry(&(hisifd->cmdlist_data_tmp[wb_compose_type]->cmdlist_head_temp[i]), dss_cmdlist_node_t, list_node);
			} else {
				cmdlist_node = list_first_entry(&(hisifd->cmdlist_data->cmdlist_head_temp[i]), dss_cmdlist_node_t, list_node);
			}

			list_addr = cmdlist_node->header_phys;
			if (g_debug_ovl_cmdlist) {
				HISI_FB_INFO("list_addr:0x%x, i=%d, ints_temp=0x%x\n", list_addr, i, ints_temp);
			}

			if (hisi_cmdlist_addr_check(hisifd, &list_addr) == false) {
				return -EINVAL;
			}

			temp |= (1 << i);

			outp32(cmdlist_base + CMDLIST_ADDR_MASK_EN, BIT(i));
			if (g_debug_set_reg_val) {
				HISI_FB_INFO("writel: [%pK] = 0x%lx\n",
					cmdlist_base + CMDLIST_ADDR_MASK_EN, BIT(i));
			}

			if (mctl_idx <= DSS_MCTL1) {
				set_reg(cmdlist_base + CMDLIST_CH0_CTRL + i * offset, 0x1, 1, 6);
			} else {
				set_reg(cmdlist_base + CMDLIST_CH0_CTRL + i * offset, 0x0, 1, 6);
			}

			set_reg(cmdlist_base + CMDLIST_CH0_STAAD + i * offset, list_addr, 32, 0);

			set_reg(cmdlist_base + CMDLIST_CH0_CTRL + i * offset, 0x1, 1, 0);
			if ((mctl_idx <= DSS_MCTL1) && ((ints_temp & 0x2) == 0x2)) {
				set_reg(cmdlist_base + CMDLIST_SWRST, 0x1, 1, i);
			}

			if (mctl_idx >= DSS_MCTL2) {
				if (((status_temp & 0xF) == 0x0) || ((ints_temp & 0x2) == 0x2)) {
					set_reg(cmdlist_base + CMDLIST_SWRST, 0x1, 1, i);
				} else {
					HISI_FB_INFO("i=%d, status_temp=0x%x, ints_temp=0x%x\n", i, status_temp, ints_temp);
				}
			}
		}

		cmdlist_idxs_temp = cmdlist_idxs_temp >> 1;
	}

	outp32(cmdlist_base + CMDLIST_ADDR_MASK_DIS, temp);
	if (g_debug_set_reg_val) {
		HISI_FB_INFO("writel: [%pK] = 0x%x\n",
			cmdlist_base + CMDLIST_ADDR_MASK_DIS, temp);
	}

	cmdlist_idxs_temp = cmdlist_idxs;
	for (i = 0; i < HISI_DSS_CMDLIST_MAX; i++) {
		if ((cmdlist_idxs_temp & 0x1) == 0x1) {
			set_reg(cmdlist_base + CMDLIST_CH0_CTRL + i * offset, mctl_idx, 3, 2);
		}

		cmdlist_idxs_temp = cmdlist_idxs_temp >> 1;
	}

	if (mctl_idx >= DSS_MCTL2) {
		set_reg(mctl_base + MCTL_CTL_ST_SEL, 0x1, 1, 0);
		set_reg(mctl_base + MCTL_CTL_SW_ST, 0x1, 1, 0);
	}

	return 0;
}


void hisi_cmdlist_config_mif_reset(struct hisi_fb_data_type *hisifd,
	dss_overlay_t *pov_req, uint32_t cmdlist_idxs, int mctl_idx)
{
	char __iomem *dss_base = NULL;
	char __iomem *tmp_base = NULL;

	uint32_t cmdlist_idxs_temp = 0;
	int delay_count = 0;
	bool is_timeout = true;
	int i = 0;
	int j = 0;
	int mif_sub_ch_nums = 4;
	int tmp = 0;
	int mif_nums_max = 0;

	if (NULL == hisifd) {
		HISI_FB_ERR("hisifd is NULL");
		return;
	}
	if (NULL == pov_req) {
		HISI_FB_ERR("pov_req is NULL");
		return;
	}

	dss_base = hisifd->dss_base;

	if (mctl_idx <= DSS_MCTL1) {
		mif_nums_max = DSS_WCHN_W0;
	} else {
		mif_nums_max = DSS_CHN_MAX;
	}

	//check mif chn status & mif ctrl0: chn disable
	if (mctl_idx == DSS_MCTL5) {
		for (i= DSS_RCHN_V2; i < DSS_CHN_MAX_DEFINE; i++) {
			is_timeout = false;

			while (1) {
				for (j = 1; j <= mif_sub_ch_nums; j++) {
					tmp |= inp32(dss_base + DSS_MIF_OFFSET + MIF_STAT1 + 0x10 * (i * mif_sub_ch_nums+ j));
				}

				if (delay_count > 500 || ((tmp & 0x1f) == 0x0)) {
					is_timeout = (delay_count > 500) ? true : false;
					delay_count = 0;
					break;
				} else {
					udelay(10);
					++delay_count;
				}
			}

			if (is_timeout) {
				HISI_FB_ERR("mif_ch%d MIF_STAT1=0x%x !\n", i, tmp);
			}
		}

		tmp_base = hisifd->dss_module.mif_ch_base[DSS_RCHN_V2];
		if (tmp_base) {
			set_reg(tmp_base + MIF_CTRL0, 0x0, 1, 0);
		}

		tmp_base = hisifd->dss_module.mif_ch_base[DSS_WCHN_W2];
		if (tmp_base) {
			set_reg(tmp_base + MIF_CTRL0, 0x0, 1, 0);
		}
	} else {
		cmdlist_idxs_temp = cmdlist_idxs;
		for (i = 0; i < mif_nums_max; i++) {
			if ((cmdlist_idxs_temp & 0x1) == 0x1) {
				is_timeout = false;

				while (1) {
					for (j = 1; j <= mif_sub_ch_nums; j++) {
						tmp |= inp32(dss_base + DSS_MIF_OFFSET + MIF_STAT1 + 0x10 * (i * mif_sub_ch_nums+ j));
					}

					if (((tmp & 0x1f) == 0x0) || delay_count > 500) {
						is_timeout = (delay_count > 500) ? true : false;
						delay_count = 0;
						break;
					} else {
						udelay(10);
						++delay_count;
					}
				}

				if (is_timeout) {
					HISI_FB_ERR("mif_ch%d MIF_STAT1=0x%x !\n", i, tmp);
				}
			}

			cmdlist_idxs_temp = cmdlist_idxs_temp >> 1;
		}

		cmdlist_idxs_temp = cmdlist_idxs;
		for (i = 0; i < mif_nums_max; i++) {
			if ((cmdlist_idxs_temp & 0x1) == 0x1) {
				tmp_base = hisifd->dss_module.mif_ch_base[i];
				if (tmp_base) {
					set_reg(tmp_base + MIF_CTRL0, 0x0, 1, 0);
				}
			}

			cmdlist_idxs_temp = cmdlist_idxs_temp >> 1;
		}
	}

	mdelay(5);

	//mif ctrl0: chn enable
	if (mctl_idx == DSS_MCTL5) {
		tmp_base = hisifd->dss_module.mif_ch_base[DSS_RCHN_V2];
		if (tmp_base) {
			set_reg(tmp_base + MIF_CTRL0, 0x1, 1, 0);
		}

		tmp_base = hisifd->dss_module.mif_ch_base[DSS_WCHN_W2];
		if (tmp_base) {
			set_reg(tmp_base + MIF_CTRL0, 0x1, 1, 0);
		}
	} else {
		cmdlist_idxs_temp = cmdlist_idxs;
		for (i = 0; i < mif_nums_max; i++) {
			if ((cmdlist_idxs_temp & 0x1) == 0x1) {
				tmp_base = hisifd->dss_module.mif_ch_base[i];
				if (tmp_base) {
					set_reg(tmp_base + MIF_CTRL0, 0x1, 1, 0);
				}
			}

			cmdlist_idxs_temp = cmdlist_idxs_temp >> 1;
		}
	}
}

/*lint -e679 -e527*/
static bool hisifb_mctl_clear_ack_timeout(char __iomem *mctl_base)
{
	uint32_t mctl_status;
	int delay_count = 0;
	bool is_timeout = false;

	while (1) {
		mctl_status = inp32(mctl_base + MCTL_CTL_STATUS);
		if (((mctl_status & 0x8) == 0) || (delay_count > 500)) {
			is_timeout = (delay_count > 500) ? true : false;
			delay_count = 0;
			break;
		} else {
			udelay(1);
			++delay_count;
		}
	}

	return is_timeout;
}

void hisifb_mctl_sw_clr(struct hisi_fb_data_type *hisifd,
	dss_overlay_t *pov_req, uint32_t cmdlist_idxs)
{
	char __iomem *mctl_base = NULL;
	char __iomem *ldi_base = NULL;
	char __iomem *cmdlist_base;
	int mctl_idx;
	uint32_t tmp = 0, i = 0;
	uint32_t isr_s1 = 0;
	uint32_t isr_s2 = 0;

	if ((hisifd == NULL) || (pov_req == NULL)) {
		HISI_FB_ERR("hisifd or pov_req is NULL!\n");
		return;
	}

	cmdlist_base = hisifd->dss_base + DSS_CMDLIST_OFFSET;
	// set cmdlist chn soft reset
	set_reg(cmdlist_base + CMDLIST_SWRST, cmdlist_idxs, 32, 0);

	mctl_idx = pov_req->ovl_idx;
	if ((mctl_idx < 0) || (mctl_idx >= DSS_MCTL_IDX_MAX)) {
		HISI_FB_ERR("mctl_idx=%d is invalid.", mctl_idx);
		return;
	}

	if (pov_req->wb_compose_type == DSS_WB_COMPOSE_MEDIACOMMON) {
		mctl_base = hisifd->media_common_base + MCTL_MUTEX_OFFSET;
	} else {
		mctl_base = hisifd->dss_module.mctl_base[mctl_idx];
	}

	if (mctl_base) {
		set_reg(mctl_base + MCTL_CTL_CLEAR, 0x1, 1, 0);
		if (hisifb_mctl_clear_ack_timeout(mctl_base)) {
			HISI_FB_ERR("clear_ack_timeout, mctl_status=0x%x !\n", inp32(mctl_base + MCTL_CTL_STATUS));
			for (i = 0; i < DSS_CHN_MAX_DEFINE; i++) {
				HISI_FB_ERR("chn%d: [DMA_BUF_DBG0]=0x%x [DMA_BUF_DBG1]=0x%x !\n", i,
					inp32(hisifd->dss_base + g_dss_module_base[i][MODULE_DMA] + DMA_BUF_DBG0),
					inp32(hisifd->dss_base + g_dss_module_base[i][MODULE_DMA] + DMA_BUF_DBG1));
			}
		}
	}

	if ((hisifd->index == PRIMARY_PANEL_IDX) || (hisifd->index == EXTERNAL_PANEL_IDX)) {
		enable_ldi(hisifd);
	}


	if (hisifd->index == PRIMARY_PANEL_IDX) {
		isr_s1 = inp32(hisifd->dss_base + GLB_CPU_PDP_INTS);
		isr_s2 = inp32(hisifd->dss_base + DSS_LDI0_OFFSET + LDI_CPU_ITF_INTS);
		outp32(hisifd->dss_base + DSS_LDI0_OFFSET + LDI_CPU_ITF_INTS, isr_s2);
		outp32(hisifd->dss_base + GLB_CPU_PDP_INTS, isr_s1);

		tmp = inp32(hisifd->dss_base + DSS_LDI0_OFFSET + LDI_CPU_ITF_INT_MSK);
		outp32(hisifd->dss_base + DSS_LDI0_OFFSET + LDI_CPU_ITF_INT_MSK, tmp);
	} else if (hisifd->index == EXTERNAL_PANEL_IDX) {
		isr_s1 = inp32(hisifd->dss_base + GLB_CPU_SDP_INTS);
		ldi_base = hisifd->dss_base + DSS_LDI1_OFFSET;
		isr_s2 = inp32(ldi_base + LDI_CPU_ITF_INTS);
		outp32(ldi_base + LDI_CPU_ITF_INTS, isr_s2);
		outp32(hisifd->dss_base + GLB_CPU_SDP_INTS, isr_s1);

		tmp = inp32(ldi_base + LDI_CPU_ITF_INT_MSK);
		outp32(ldi_base + LDI_CPU_ITF_INT_MSK, tmp);
	}
}

void hisi_mctl_ctl_clear(struct hisi_fb_data_type *hisifd, int mctl_idx)
{
	char __iomem *tmp_base;

	if (hisifd == NULL) {
		HISI_FB_ERR("hisifd is NULL point!\n");
		return;
	}

	if ((mctl_idx < 0) || (mctl_idx >= DSS_MCTL_IDX_MAX)) {
		HISI_FB_ERR("mctl_idx=%d is invalid!\n", mctl_idx);
		return;
	}

	tmp_base = hisifd->dss_module.mctl_base[mctl_idx];
	if (tmp_base) {
		set_reg(tmp_base + MCTL_CTL_CLEAR, 0x1, 1, 0);
		if (hisifb_mctl_clear_ack_timeout(tmp_base)) {
			HISI_FB_ERR("clear_ack_timeout, mctl_status =0x%x !\n", inp32(tmp_base + MCTL_CTL_STATUS));
		}
	}
}

void hisi_cmdlist_config_reset(struct hisi_fb_data_type *hisifd,
	dss_overlay_t *pov_req, uint32_t cmdlist_idxs)
{
	char __iomem *dss_base = NULL;
	char __iomem *cmdlist_base = NULL;
	char __iomem *tmp_base = NULL;
	char __iomem *ov_base = NULL;
	char __iomem *mctl_sys_base = NULL;
	struct hisi_panel_info *pinfo = NULL;

	uint32_t offset = 0;
	uint32_t cmdlist_idxs_temp = 0;
	int delay_count = 0;
	bool is_timeout = true;
	int i = 0;
	int ovl_idx = 0;
	int mctl_idx = 0;
	int tmp = 0;
	int ints_temp;
	int start_sel;
	uint32_t start_sel_temp;

	if (hisifd == NULL) {
		HISI_FB_ERR("hisifd is NULL point!\n");
		return;
	}

	HISI_FB_DEBUG("fb%d, +.\n", hisifd->index);

	if (pov_req == NULL) {
		HISI_FB_ERR("pov_req is NULL point!\n");
		return;
	}


	dss_base = hisifd->dss_base;
	cmdlist_base = dss_base + DSS_CMDLIST_OFFSET;
	ovl_idx = pov_req->ovl_idx;
	ints_temp = 0;
	start_sel = 0;
	start_sel_temp = 0;
	pinfo = &(hisifd->panel_info);

	if (cmdlist_idxs == 0) {
		return;
	}
	mctl_idx = ovl_idx;

	if ((mctl_idx < 0) || (mctl_idx >= DSS_MCTL_IDX_MAX)) {
		HISI_FB_ERR("mctl_idx=%d is invalid.", mctl_idx);
		return;
	}


	down(&hisifd->hiace_clear_sem);
	//hisifb_check_idle_ctrl_wb_clr(hisifd);

	if (pov_req->wb_compose_type == DSS_WB_COMPOSE_COPYBIT) {
		mctl_idx = DSS_MCTL5;
	}

	offset = 0x40;
	cmdlist_idxs_temp = HISI_DSS_CMDLIST_IDXS_MAX;
	for (i = 0; i < HISI_DSS_CMDLIST_MAX; i++) {
		if ((cmdlist_idxs_temp & 0x1) == 0x1) {
			ints_temp = inp32(cmdlist_base + CMDLIST_CH0_INTS + i * offset);
			start_sel = inp32(cmdlist_base + CMDLIST_CH0_CTRL + i * offset);

			if (((ints_temp & 0x2) == 0x2) && ((start_sel & 0x1c) == 0)) {
				set_reg(cmdlist_base + CMDLIST_CH0_CTRL + i * offset, 0x6, 3, 2);
				start_sel_temp |= (1 << i);
			}
		}

		cmdlist_idxs_temp = cmdlist_idxs_temp >> 1;
	}

	//set cmdlist chn soft reset
	set_reg(cmdlist_base+CMDLIST_SWRST,cmdlist_idxs,32,0);

	//MCTL_CTL_CLEAR
	hisi_mctl_ctl_clear(hisifd, mctl_idx);

	hisi_cmdlist_config_mif_reset(hisifd, pov_req, cmdlist_idxs, mctl_idx);

	cmdlist_idxs_temp = start_sel_temp;
	for (i = 0; i < HISI_DSS_CMDLIST_MAX; i++) {
		if ((cmdlist_idxs_temp & 0x1) == 0x1) {
			set_reg(cmdlist_base + CMDLIST_CH0_CTRL + i * offset, mctl_idx, 3, 2);
		}

		cmdlist_idxs_temp = cmdlist_idxs_temp >> 1;
	}

	if (mctl_idx >= DSS_MCTL2) {
		offset = 0x40;
		cmdlist_idxs_temp = cmdlist_idxs;
		for (i = 0; i < HISI_DSS_CMDLIST_MAX; i++) {
			if ((cmdlist_idxs_temp & 0x1) == 0x1) {
				set_reg(cmdlist_base + CMDLIST_CH0_CTRL + i * offset, 0x6, 3, 2);
				set_reg(cmdlist_base + CMDLIST_CH0_CTRL + i * offset, 0x0, 1, 0);
			}

			cmdlist_idxs_temp = cmdlist_idxs_temp >> 1;
		}
	}
	up(&hisifd->hiace_clear_sem);

	return;

	// set  cmdlist chn pause enter
	offset = 0x40;
	cmdlist_idxs_temp = cmdlist_idxs;
	for (i = 0; i < HISI_DSS_CMDLIST_MAX; i++) {
		if ((cmdlist_idxs_temp & 0x1) == 0x1) {
			set_reg(cmdlist_base + CMDLIST_CH0_CTRL + i * offset, 0x1, 1, 3);
		}

		cmdlist_idxs_temp = cmdlist_idxs_temp >> 1;
	}

	mdelay(1);

	// wait cmdlist chn idle (oa_idle ch_idle)
	offset = 0x40;
	cmdlist_idxs_temp = cmdlist_idxs;
	for (i = 0; i < HISI_DSS_CMDLIST_MAX; i++) {
		if ((cmdlist_idxs_temp & 0x1) == 0x1) {
			is_timeout = false;

			while (1) {
				tmp = inp32(cmdlist_base + CMDLIST_CH0_STATUS + i * offset);
				if (((tmp & 0x400) == 0x400) || delay_count > 500) {
					is_timeout = (delay_count > 500) ? true : false;
					delay_count = 0;
					break;
				} else {
					udelay(10);
					++delay_count;
				}
			}

			if (is_timeout) {
				HISI_FB_ERR("cmdlist_ch%d can not exit to idle!CMDLIST_CH0_STATUS=0x%x !\n", i, tmp);
			}
		}

		cmdlist_idxs_temp = cmdlist_idxs_temp >> 1;
	}

	// set cmdlist chn soft reset
	set_reg(cmdlist_base + CMDLIST_SWRST, cmdlist_idxs, 32, 0);

	//offline need to restart
	g_offline_cmdlist_idxs = 0;

	mdelay(1);
	//////////////////////////////online composer//////////////////////////////////
	if (ovl_idx < DSS_OVL2) {
		// MCTL_MUTEX, MCTL_CTL_CLEAR
		tmp_base = hisifd->dss_module.mctl_base[mctl_idx];
		if (tmp_base) {
			set_reg(tmp_base + MCTL_CTL_CLEAR, 0x1, 1, 0);
		}


 		hisi_cmdlist_config_mif_reset(hisifd, pov_req, cmdlist_idxs, mctl_idx);

		if (ovl_idx == DSS_OVL0) {
			if (is_mipi_video_panel(hisifd) || (g_ldi_data_gate_en == 0)) {
				ov_base = hisifd->dss_module.ov_base[ovl_idx];
				set_reg(ov_base + OVL6_REG_DEFAULT, 0x1, 32, 0);
				set_reg(ov_base + OVL6_REG_DEFAULT, 0x0, 32, 0);

				mctl_sys_base = hisifd->dss_module.mctl_sys_base;
				set_reg(mctl_sys_base + MCTL_OV0_FLUSH_EN, 0xf, 32, 0);
				tmp = inp32(mctl_sys_base + MCTL_MOD17_DBG);
				if ((tmp | 0x2) == 0x0) {
					HISI_FB_INFO("itf0 flush_en status invaild, MCTL_MOD17_DBG=0x%x\n", tmp);
				}

			}
		}

		// MCTL_MUTEX, MCTL_CTL_CLEAR
		tmp_base = hisifd->dss_module.mctl_base[mctl_idx];
		if (tmp_base) {
			set_reg(tmp_base + MCTL_CTL_CLEAR, 0x1, 1, 0);
		}
	} else {
	//////////////////////////////offline composer//////////////////////////////////
		// MCTL_MUTEX, MCTL_CTL_CLEAR
		tmp_base = hisifd->dss_module.mctl_base[mctl_idx];
		if (tmp_base) {
			set_reg(tmp_base + MCTL_CTL_CLEAR, 0x1, 1, 0);
		}


		hisi_cmdlist_config_mif_reset(hisifd, pov_req, cmdlist_idxs, mctl_idx);

		// MCTL_MUTEX, MCTL_CTL_CLEAR
		tmp_base = hisifd->dss_module.mctl_base[mctl_idx];
		if (tmp_base) {
			set_reg(tmp_base + MCTL_CTL_CLEAR, 0x1, 1, 0);
		}
	}

	// set cmdlist chn pause exit
	offset = 0x40;
	cmdlist_idxs_temp = cmdlist_idxs;
	for (i = 0; i < HISI_DSS_CMDLIST_MAX; i++) {
		if ((cmdlist_idxs_temp & 0x1) == 0x1) {
			set_reg(cmdlist_base + CMDLIST_CH0_CTRL + i * offset, 0x0, 1, 3);
		}

		cmdlist_idxs_temp = cmdlist_idxs_temp >> 1;
	}

	HISI_FB_DEBUG("fb%d, -.\n", hisifd->index);
}

int hisi_cmdlist_config_stop(struct hisi_fb_data_type *hisifd, uint32_t cmdlist_pre_idxs)
{
	dss_overlay_t *pov_req = NULL;
	char __iomem *cmdlist_base = NULL;
	int i = 0;
	uint32_t tmp = 0;
	uint32_t offset = 0;

	int ret = 0;

	if (NULL == hisifd) {
		HISI_FB_ERR("hisifd is NULL");
		return -EINVAL;
	}

	pov_req = &(hisifd->ov_req);

	if ((pov_req->ovl_idx < 0) ||
		pov_req->ovl_idx >= DSS_OVL_IDX_MAX) {
		HISI_FB_ERR("fb%d, invalid ovl_idx=%d!",
			hisifd->index, pov_req->ovl_idx);
		goto err_return;
	}

	cmdlist_base = hisifd->dss_base + DSS_CMDLIST_OFFSET;
	offset = 0x40;
	//remove prev chn cmdlist
	ret = hisi_cmdlist_add_new_node(hisifd, cmdlist_pre_idxs, 0, 1, 1, 1, 0);
	if (ret != 0) {
		HISI_FB_ERR("fb%d, hisi_cmdlist_add_new_node err:%d \n", hisifd->index, ret);
		goto err_return;
	}

	for (i = 0; i < DSS_WCHN_W0; i++) {
		tmp = (0x1 << i);
		hisifd->cmdlist_idx = i;

		if ((cmdlist_pre_idxs & tmp) == tmp) {
			hisifd->set_reg(hisifd, hisifd->dss_module.mctl_base[pov_req->ovl_idx] +
				MCTL_CTL_MUTEX_RCH0 + i * 0x4, 0, 32, 0);
			hisifd->set_reg(hisifd, cmdlist_base + CMDLIST_CH0_CTRL + i * offset, 0x6, 3, 2);
		}
	}

	if ((cmdlist_pre_idxs & (0x1 << DSS_CMDLIST_V2)) == (0x1 << DSS_CMDLIST_V2)) {
		hisifd->cmdlist_idx = DSS_CMDLIST_V2;
		hisifd->set_reg(hisifd, hisifd->dss_module.mctl_base[pov_req->ovl_idx] +
			MCTL_CTL_MUTEX_RCH8, 0, 32, 0);
		hisifd->set_reg(hisifd, cmdlist_base + CMDLIST_CH0_CTRL + DSS_CMDLIST_V2 * offset, 0x6, 3, 2);
	}

	return 0;

err_return:
	return ret;
}
/*lint +e679 +e527*/

void hisi_dss_cmdlist_qos_on(struct hisi_fb_data_type *hisifd)
{
	char __iomem *cmdlist_base = NULL;

	if (NULL == hisifd) {
		HISI_FB_ERR("hisifd is NULL");
		return;
	}

	cmdlist_base = hisifd->dss_base + DSS_CMDLIST_OFFSET;
	set_reg(cmdlist_base + CMDLIST_CTRL, 0x3, 2, 4);
}

void hisi_dump_cmdlist_node_items(cmd_item_t * item, uint32_t count)
{
	uint32_t index = 0;
	uint32_t addr = 0;

	for (index = 0; index < count; index++) {
		addr = item[index].reg_addr.bits.add0;
		addr = addr & CMDLIST_ADDR_OFFSET;
		addr = addr << 2;
		HISI_FB_INFO("set addr:0x%x value:0x%x add1:0x%x value:0x%x add2:0x%x value:0x%x \n",
			addr, item[index].data0,
			item[index].reg_addr.bits.add1 << 2, item[index].data1,
			item[index].reg_addr.bits.add2 <<2 , item[index].data2);
	}
}

static void hisi_dump_cmdlist_content(struct list_head *cmdlist_head, char *filename, uint32_t addr)
{
	dss_cmdlist_node_t *node = NULL;
	dss_cmdlist_node_t *_node_ = NULL;

	if (NULL == cmdlist_head) {
		HISI_FB_ERR("cmdlist_head is NULL");
		return;
	}
	if (NULL == filename) {
		HISI_FB_ERR("filename is NULL");
		return;
	}

	if (g_dump_cmdlist_content == 0)
		return ;

	HISI_FB_INFO("%s\n", filename);

	list_for_each_entry_safe(node, _node_, cmdlist_head, list_node) {
		if (node->header_phys == addr) {
			hisifb_save_file(filename, (char *)(node->list_header), CMDLIST_HEADER_LEN);
		}

		if (node->item_phys == addr) {
			hisifb_save_file(filename, (char *)(node->list_item), CMDLIST_ITEM_LEN);
		}
	}
}

static void hisi_dump_cmdlist_one_node (struct list_head *cmdlist_head, uint32_t cmdlist_idx)
{
	dss_cmdlist_node_t *node = NULL;
	dss_cmdlist_node_t *_node_ = NULL;
	uint32_t count = 0;
	int i = 0;
	char filename[256] = {0};

	if (NULL == cmdlist_head) {
		HISI_FB_ERR("cmdlist_head is NULL");
		return;
	}

	list_for_each_entry_safe(node, _node_, cmdlist_head, list_node) {
		if (node->node_type == CMDLIST_NODE_NOP) {
			HISI_FB_INFO("node type = NOP node\n");
		} else if (node->node_type == CMDLIST_NODE_FRAME) {
			HISI_FB_INFO("node type = Frame node\n");
		}

		HISI_FB_INFO("\t qos  | flag | pending | tast_end | last  | event_list | list_addr  | next_list  | count | id | is_used | reserved | cmdlist_idx\n");
		HISI_FB_INFO("\t------+---------+------------+------------+------------+------------\n");
		HISI_FB_INFO("\t 0x%2x | 0x%2x |0x%6x | 0x%5x | 0x%3x | 0x%8x | 0x%8x | 0x%8x | 0x%3x | 0x%2x | 0x%2x | 0x%2x | 0x%2x\n",
			node->list_header->flag.bits.qos, node->list_header->flag.bits.valid_flag, node->list_header->flag.bits.pending,
			node->list_header->flag.bits.task_end, node->list_header->flag.bits.last,
			node->list_header->flag.bits.event_list,
			node->list_header->list_addr, node->list_header->next_list,
			node->list_header->total_items.bits.count, node->list_header->flag.bits.id,
			node->is_used, node->reserved, cmdlist_idx);

		if (i == 0) {
			snprintf(filename, 256, "/data/dssdump/list_start_0x%x.txt", (uint32_t)node->header_phys);
			hisi_dump_cmdlist_content(cmdlist_head, filename, node->header_phys);
		}

		count = node->list_header->total_items.bits.count;
		hisi_dump_cmdlist_node_items(node->list_item, count);

		i++;
	}
}

int hisi_cmdlist_dump_all_node (struct hisi_fb_data_type *hisifd, dss_overlay_t *pov_req,
	uint32_t cmdlist_idxs)
{
	int i = 0;
	uint32_t cmdlist_idxs_temp = 0;
	uint32_t wb_compose_type = 0;

	if (NULL == hisifd) {
		HISI_FB_ERR("hisifd is NULL");
		return -EINVAL;
	}

	if (pov_req) {
		if (pov_req->wb_enable)
			wb_compose_type = pov_req->wb_compose_type;
	}

	cmdlist_idxs_temp = cmdlist_idxs;
	for (i = 0; i < HISI_DSS_CMDLIST_MAX; i++) {
		if (0x1 == (cmdlist_idxs_temp & 0x1)) {
			if (pov_req && pov_req->wb_enable) {
				if (hisifd->index == MEDIACOMMON_PANEL_IDX) {
					hisi_dump_cmdlist_one_node(&(hisifd->media_common_cmdlist_data->cmdlist_head_temp[i]), i);//lint !e732
				} else {
					hisi_dump_cmdlist_one_node(&(hisifd->cmdlist_data_tmp[wb_compose_type]->cmdlist_head_temp[i]), i);
				}
			} else {
				hisi_dump_cmdlist_one_node(&(hisifd->cmdlist_data->cmdlist_head_temp[i]), i);
			}
		}

		cmdlist_idxs_temp = cmdlist_idxs_temp >> 1;
	}

	return 0;
}

int hisi_cmdlist_del_node (struct hisi_fb_data_type *hisifd, dss_overlay_t *pov_req,
	uint32_t cmdlist_idxs)
{
	int i = 0;
	uint32_t wb_compose_type = 0;
	uint32_t cmdlist_idxs_temp = 0;

	if (NULL == hisifd) {
		HISI_FB_ERR("hisifd is NULL");
		return -EINVAL;
	}

	if (pov_req) {
		if (pov_req->wb_enable)
			wb_compose_type = pov_req->wb_compose_type;
	}

	cmdlist_idxs_temp = cmdlist_idxs;
	for (i = 0; i < HISI_DSS_CMDLIST_MAX; i++) {
		if ((cmdlist_idxs_temp & 0x1) == 0x1) {
			if (pov_req && pov_req->wb_enable) {
				if (hisifd->index == MEDIACOMMON_PANEL_IDX) {
					hisi_cmdlist_del_all_node(&(hisifd->media_common_cmdlist_data->cmdlist_head_temp[i]));
				} else {
					hisi_cmdlist_del_all_node(&(hisifd->cmdlist_data_tmp[wb_compose_type]->cmdlist_head_temp[i]));
				}
			} else {
				if (hisifd->cmdlist_data) {
					hisi_cmdlist_del_all_node(&(hisifd->cmdlist_data->cmdlist_head_temp[i]));
				}
			}
		}

		cmdlist_idxs_temp = (cmdlist_idxs_temp >> 1);
	}

	return 0;
}

static dss_cmdlist_data_t* hisi_cmdlist_data_alloc(struct hisi_fb_data_type *hisifd)
{
	int i = 0;
	int j = 0;
	dss_cmdlist_data_t *cmdlist_data = NULL;

	if (NULL == hisifd) {
		HISI_FB_ERR("hisifd is NULL");
		return NULL;
	}

	cmdlist_data = (dss_cmdlist_data_t *)kmalloc(sizeof(dss_cmdlist_data_t), GFP_ATOMIC);
	if (cmdlist_data) {
		memset(cmdlist_data, 0, sizeof(dss_cmdlist_data_t));
	} else {
		HISI_FB_ERR("failed to kmalloc cmdlist_data!\n");
		return NULL;
	}

	for (i = 0; i < HISI_DSS_CMDLIST_MAX; i++) {
		INIT_LIST_HEAD(&(cmdlist_data->cmdlist_head_temp[i]));

		for (j=0; j < HISI_DSS_CMDLIST_NODE_MAX; j++) {
			cmdlist_data->cmdlist_nodes_temp[i][j] = hisi_cmdlist_node_alloc(hisifd);
			if (cmdlist_data->cmdlist_nodes_temp[i][j] == NULL) {
				HISI_FB_ERR("failed to hisi_cmdlist_node_alloc!\n");
				kfree(cmdlist_data);
				return NULL;
			}
		}
	}

	return cmdlist_data;
}

static void hisi_cmdlist_data_free(struct hisi_fb_data_type *hisifd, dss_cmdlist_data_t *cmdlist_data)
{
	int i = 0;
	int j = 0;

	if (NULL == hisifd) {
		HISI_FB_ERR("hisifd is NULL");
		return;
	}
	if (NULL == cmdlist_data) {
		HISI_FB_ERR("cmdlist_data is NULL");
		return;
	}

	for (i= 0; i < HISI_DSS_CMDLIST_MAX; i++) {
		for (j = 0; j < HISI_DSS_CMDLIST_NODE_MAX; j++) {
			hisi_cmdlist_node_free(hisifd, cmdlist_data->cmdlist_nodes_temp[i][j]);
			cmdlist_data->cmdlist_nodes_temp[i][j] = NULL;
		}
	}

	kfree(cmdlist_data);
	cmdlist_data = NULL;
}

static dss_cmdlist_info_t* hisi_cmdlist_info_alloc(struct hisi_fb_data_type *hisifd)
{
	int i = 0;
	dss_cmdlist_info_t *cmdlist_info = NULL;

	if (NULL == hisifd) {
		HISI_FB_ERR("hisifd is NULL");
		return NULL;
	}

	cmdlist_info = (dss_cmdlist_info_t *)kmalloc(sizeof(dss_cmdlist_info_t), GFP_ATOMIC);
	if (cmdlist_info) {
		memset(cmdlist_info, 0, sizeof(dss_cmdlist_info_t));
	} else {
		HISI_FB_ERR("failed to kmalloc cmdlist_info!\n");
		return NULL;
	}

	sema_init(&(cmdlist_info->cmdlist_wb_common_sem), 1);

	for (i = 0; i < WB_TYPE_MAX; i++) {
		sema_init(&(cmdlist_info->cmdlist_wb_sem[i]), 1);
		init_waitqueue_head(&(cmdlist_info->cmdlist_wb_wq[i]));
		cmdlist_info->cmdlist_wb_done[i] = 0;
		cmdlist_info->cmdlist_wb_flag[i] = 0;
	}

	return cmdlist_info;
}

static dss_copybit_info_t* hisi_copybit_info_alloc(struct hisi_fb_data_type *hisifd)
{
	dss_copybit_info_t *copybit_info = NULL;

	if (NULL == hisifd) {
		HISI_FB_ERR("hisifd is NULL");
		return NULL;
	}

	copybit_info = (dss_copybit_info_t *)kmalloc(sizeof(dss_copybit_info_t), GFP_ATOMIC);
	if (copybit_info) {
		memset(copybit_info, 0, sizeof(dss_copybit_info_t));
	} else {
		HISI_FB_ERR("failed to kmalloc copybit_info!\n");
		return NULL;
	}

	sema_init(&(copybit_info->copybit_sem), 1);

	init_waitqueue_head(&(copybit_info->copybit_wq));
	copybit_info->copybit_done = 0;

	return copybit_info;
}

static dss_media_common_info_t* hisi_media_common_info_alloc(struct hisi_fb_data_type *hisifd)
{
	dss_media_common_info_t *mdc_info = NULL;//lint !e838

	if (NULL == hisifd) {
		HISI_FB_ERR("hisifd is null.\n");
	}

	mdc_info = (dss_media_common_info_t *)kmalloc(sizeof(dss_media_common_info_t), GFP_ATOMIC);
	if (mdc_info) {
		memset(mdc_info, 0, sizeof(dss_media_common_info_t));
	} else {
		HISI_FB_ERR("failed to kmalloc copybit_info!\n");
		return NULL;
	}

	init_waitqueue_head(&(mdc_info->mdc_wq));
	mdc_info->mdc_done = 0;

	return mdc_info;
}

void hisi_cmdlist_data_get_online(struct hisi_fb_data_type *hisifd)
{
	int tmp = 0;

	if (NULL == hisifd) {
		HISI_FB_ERR("hisifd is NULL");
		return;
	}

	tmp = hisifd->frame_count % HISI_DSS_CMDLIST_DATA_MAX;
	hisifd->cmdlist_data = hisifd->cmdlist_data_tmp[tmp];
	hisi_cmdlist_del_node(hisifd, NULL, HISI_DSS_CMDLIST_IDXS_MAX);
}

void hisi_cmdlist_data_get_offline(struct hisi_fb_data_type *hisifd, dss_overlay_t *pov_req)
{
	if (NULL == hisifd) {
		HISI_FB_ERR("hisifd is NULL");
		return;
	}
	if (NULL == pov_req) {
		HISI_FB_ERR("pov_req is NULL");
		return;
	}

	if (pov_req->wb_compose_type == DSS_WB_COMPOSE_COPYBIT) {
		hisifd->cmdlist_data = hisifd->cmdlist_data_tmp[1];
	} else {
		hisifd->cmdlist_data = hisifd->cmdlist_data_tmp[0];
	}

	if (!hisifd->cmdlist_data) {
		HISI_FB_ERR("fb%d, cmdlist_data is NULL!\n", hisifd->index);
	}
}

static int hisi_cmdlist_pool_init(struct hisi_fb_data_type *hisifd)
{
	int ret = 0;
	int i = 0;
	int j = 0;
	size_t one_cmdlist_pool_size = 0;
	size_t tmp = 0;

	if (!hisifd || !(hisifd->pdev)) {
		HISI_FB_ERR("hisifd is NULL!\n");
		return -EINVAL;
	}

	for (i = 0; i < HISI_DSS_CMDLIST_MAX; i++) {
		for (j=0; j < HISI_DSS_CMDLIST_NODE_MAX; j++) {
			one_cmdlist_pool_size += (roundup(CMDLIST_HEADER_LEN, PAGE_SIZE) +
				roundup(CMDLIST_ITEM_LEN, PAGE_SIZE));
		}
	}

	if (hisifd->index == AUXILIARY_PANEL_IDX) {
		hisifd->sum_cmdlist_pool_size = 2 * one_cmdlist_pool_size;
	} else if (hisifd->index == MEDIACOMMON_PANEL_IDX) {
		hisifd->sum_cmdlist_pool_size = one_cmdlist_pool_size;
	} else if (hisifd->index == EXTERNAL_PANEL_IDX && !hisifd->panel_info.fake_external) {
		hisifd->sum_cmdlist_pool_size = HISI_DSS_CMDLIST_DATA_MAX * one_cmdlist_pool_size;
	} else {
		if (hisifd->index == PRIMARY_PANEL_IDX) {
			for (i = 0; i < HISI_DSS_CMDLIST_DATA_MAX; i++) {
				hisifd->sum_cmdlist_pool_size += one_cmdlist_pool_size;
			}
		}
	}

	if (hisifd->sum_cmdlist_pool_size == 0) {
		return 0;
	}

	/*alloc cmdlist pool buffer*/
	hisifd->cmdlist_pool_ion_handle = ion_alloc(hisifd->ion_client, hisifd->sum_cmdlist_pool_size, 0, ION_HEAP(ION_GRALLOC_HEAP_ID), 0);
	if (IS_ERR(hisifd->cmdlist_pool_ion_handle)) {
		HISI_FB_ERR("failed to ion alloc cmdlist_ion_handle!");
		ret = -ENOMEM;
		goto err_ion_handle;
	}

	hisifd->cmdlist_pool_vir_addr = ion_map_kernel(hisifd->ion_client, hisifd->cmdlist_pool_ion_handle);
	if (!hisifd->cmdlist_pool_vir_addr ) {
		HISI_FB_ERR("failed to ion_map_kernel cmdlist_pool_vir_addr!");
		ret = -ENOMEM;
		goto err_ion_map;
	}
	memset(hisifd->cmdlist_pool_vir_addr, 0, hisifd->sum_cmdlist_pool_size);

	ret = hisifb_ion_phys(hisifd->ion_client, hisifd->cmdlist_pool_ion_handle, &(hisifd->pdev->dev), &(hisifd->cmdlist_pool_phy_addr), &tmp);
	if (ret < 0) {
		HISI_FB_ERR("failed to ion_phys node->header_phys!");
		ret = -ENOMEM;
		goto err_ion_phys;
	}
	HISI_FB_INFO("fb%d,  sum_cmdlist_pool_size=%zu, tmp=%zu.\n",
		hisifd->index, hisifd->sum_cmdlist_pool_size, tmp);

	/* create cmdlist pool */
	hisifd->cmdlist_pool = gen_pool_create(PAGE_SHIFT, -1);
	if (hisifd->cmdlist_pool  == NULL) {
		HISI_FB_ERR("fb%d, cmdlist_pool gen_pool_create failed!", hisifd->index);
		ret = -ENOMEM;
		goto err_pool_create;
	}

	if (gen_pool_add_virt(hisifd->cmdlist_pool, (unsigned long)hisifd->cmdlist_pool_vir_addr,
		hisifd->cmdlist_pool_phy_addr, hisifd->sum_cmdlist_pool_size, -1) != 0) {
		HISI_FB_ERR("fb%d, cmdlist_pool gen_pool_add failed!", hisifd->index);
		goto err_pool_add;
	}

	return 0;

err_pool_add:
	if (hisifd->cmdlist_pool) {
		gen_pool_destroy(hisifd->cmdlist_pool);
		hisifd->cmdlist_pool = NULL;
	}

err_pool_create:
err_ion_phys:
	if (hisifd->cmdlist_pool_ion_handle) {
		ion_unmap_kernel(hisifd->ion_client, hisifd->cmdlist_pool_ion_handle);
	}
err_ion_map:
	if (hisifd->cmdlist_pool_ion_handle) {
		ion_free(hisifd->ion_client, hisifd->cmdlist_pool_ion_handle);
		hisifd->cmdlist_pool_ion_handle = NULL;
	}
err_ion_handle:
	hisifd->cmdlist_pool_ion_handle = NULL;
	return ret;
}

static void hisi_cmdlist_pool_deinit(struct hisi_fb_data_type *hisifd)
{
	if (!hisifd) {
		HISI_FB_ERR("hisifd is NULL!\n");
		return ;
	}

	if (hisifd->cmdlist_pool) {
		gen_pool_destroy(hisifd->cmdlist_pool);
		hisifd->cmdlist_pool = NULL;
	}

	if (hisifd->cmdlist_pool_ion_handle) {
		ion_unmap_kernel(hisifd->ion_client, hisifd->cmdlist_pool_ion_handle);
		ion_free(hisifd->ion_client, hisifd->cmdlist_pool_ion_handle);
		hisifd->cmdlist_pool_ion_handle = NULL;
	}
}

int hisi_cmdlist_init(struct hisi_fb_data_type *hisifd)
{
	int ret = 0;
	int i = 0;

	if (NULL == hisifd) {
		HISI_FB_ERR("hisifd is NULL");
		return -EINVAL;
	}

	ret = hisi_cmdlist_pool_init(hisifd);
	if (ret != 0) {
		HISI_FB_ERR("fb%d, hisi_cmdlist_pool_init failed!\n", hisifd->index);
		return -EINVAL;
	}

	for (i = 0; i < HISI_DSS_CMDLIST_BLOCK_MAX; i++) {
		hisifd->ov_block_rects[i] = (dss_rect_t *)kmalloc(sizeof(dss_rect_t), GFP_ATOMIC);
		if (!hisifd->ov_block_rects[i]) {
			HISI_FB_ERR("ov_block_rects[%d] failed to alloc!", i);
			hisi_cmdlist_pool_deinit(hisifd);
			return -EINVAL;
		}
	}

	if (hisifd->index == AUXILIARY_PANEL_IDX) {
		hisifd->cmdlist_data_tmp[0] = hisi_cmdlist_data_alloc(hisifd);
		hisifd->cmdlist_data_tmp[1] = hisi_cmdlist_data_alloc(hisifd);
		hisifd->cmdlist_info = hisi_cmdlist_info_alloc(hisifd);
		hisifd->copybit_info = hisi_copybit_info_alloc(hisifd);
	} else if (hisifd->index == MEDIACOMMON_PANEL_IDX) {
		hisifd->media_common_cmdlist_data = hisi_cmdlist_data_alloc(hisifd);
		hisifd->media_common_info = hisi_media_common_info_alloc(hisifd);
	} else {
		if (hisifd->index == PRIMARY_PANEL_IDX ||
			(hisifd->index == EXTERNAL_PANEL_IDX && !hisifd->panel_info.fake_external)) {
			for (i = 0; i < HISI_DSS_CMDLIST_DATA_MAX; i++) {
				hisifd->cmdlist_data_tmp[i] = hisi_cmdlist_data_alloc(hisifd);
			}
		}
	}

	hisifd->cmdlist_data = hisifd->cmdlist_data_tmp[0];
	hisifd->cmdlist_idx = -1;

	return ret;
}

int hisi_cmdlist_deinit(struct hisi_fb_data_type *hisifd)
{
	int i = 0;

	if (NULL == hisifd) {
		HISI_FB_ERR("hisifd is NULL");
		return -EINVAL;
	}

	if (hisifd->index == AUXILIARY_PANEL_IDX) {
		if (hisifd->cmdlist_info) {
			kfree(hisifd->cmdlist_info);
			hisifd->cmdlist_info = NULL;
		}

		if (hisifd->copybit_info) {
			kfree(hisifd->copybit_info);
			hisifd->copybit_info = NULL;
		}
	} else if (hisifd->index == MEDIACOMMON_PANEL_IDX) {
		if (hisifd->cmdlist_info) {
			kfree(hisifd->cmdlist_info);
			hisifd->cmdlist_info = NULL;
		}

		if (hisifd->media_common_info) {
			kfree(hisifd->media_common_info);
			hisifd->media_common_info = NULL;
		}
	} else {
		if (hisifd->index == PRIMARY_PANEL_IDX ||
			(hisifd->index == EXTERNAL_PANEL_IDX && !hisifd->panel_info.fake_external)) {
			for (i = 0; i < HISI_DSS_CMDLIST_DATA_MAX; i++) {
				if (hisifd->cmdlist_data_tmp[i]) {
					hisi_cmdlist_data_free(hisifd, hisifd->cmdlist_data_tmp[i]);
					hisifd->cmdlist_data_tmp[i] = NULL;
				}
			}
		}
	}

	for (i = 0; i < HISI_DSS_CMDLIST_BLOCK_MAX; i++) {
		if (hisifd->ov_block_rects[i]) {
			kfree(hisifd->ov_block_rects[i]);
			hisifd->ov_block_rects[i] = NULL;
		}
	}

	hisi_cmdlist_pool_deinit(hisifd);

	return 0;
}
