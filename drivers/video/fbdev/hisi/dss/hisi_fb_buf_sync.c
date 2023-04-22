/* Copyright (c) 2008-2014, Hisilicon Tech. Co., Ltd. All rights reserved.
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

#define HISI_DSS_LAYERBUF_FREE	"hisifb%d-layerbuf-free"
////////////////////////////////////////////////////////////////////////////////
//
// layerbuffer handle, for online compose
//
static bool  hisi_check_parameter_valid(struct hisi_fb_data_type *hisifd,
	dss_overlay_t *pov_req, struct list_head *plock_list)
{
	if (NULL == hisifd) {
		HISI_FB_ERR("hisifd is NULL!\n");
		return false;
	}
	if (NULL == pov_req) {
		HISI_FB_ERR("pov_req is NULL!\n");
		return false;
	}
	if (NULL == plock_list) {
		HISI_FB_ERR("plock_list is NULL!\n");
		return false;
	}
	if (NULL == hisifd->ion_client) {
		HISI_FB_ERR("ion_client is NULL!\n");
		return false;
	}

	return true;
}

int hisifb_layerbuf_lock(struct hisi_fb_data_type *hisifd,
	dss_overlay_t *pov_req, struct list_head *plock_list)
{
	dss_overlay_block_t *pov_h_block_infos = NULL;
	dss_overlay_block_t *pov_h_block = NULL;
	dss_layer_t *layer = NULL;
	int i = 0;
	int m = 0;
	struct hisifb_layerbuf *node = NULL;
	struct ion_handle *ionhnd = NULL;
	struct iommu_map_format iommu_format;
	bool add_tail = false;
	bool has_map_iommu;
	bool parameter_valid;

	parameter_valid = hisi_check_parameter_valid(hisifd, pov_req, plock_list);
	if (parameter_valid == false) {
		return -EINVAL;
	}

	pov_h_block_infos = (dss_overlay_block_t *)(pov_req->ov_block_infos_ptr);
	for (m = 0; m < pov_req->ov_block_nums; m++) {
		pov_h_block = &(pov_h_block_infos[m]);

		for (i = 0; i < pov_h_block->layer_nums; i++) {
			layer = &(pov_h_block->layer_infos[i]);
			add_tail = false;
			ionhnd = NULL;
			has_map_iommu = false;

			if (layer->dst_rect.y < pov_h_block->ov_block_rect.y)
				continue;

			if (layer->img.shared_fd < 0)
				continue;

			if ((layer->img.phy_addr == 0) &&
				(layer->img.vir_addr == 0) &&
				(layer->img.afbc_payload_addr == 0)) {
				HISI_FB_ERR("fb%d, layer_idx%d, chn_idx%d, no buffer!\n",
					hisifd->index, layer->layer_idx, layer->chn_idx);
				continue;
			}

			if (layer->img.shared_fd >= 0) {
			#if LINUX_VERSION_CODE >= KERNEL_VERSION(4,9,0)
				ionhnd = ion_import_dma_buf_fd(hisifd->ion_client, layer->img.shared_fd);
			#else
				ionhnd = ion_import_dma_buf(hisifd->ion_client, layer->img.shared_fd);
			#endif
				if (IS_ERR(ionhnd)) {
					ionhnd = NULL;
					HISI_FB_ERR("fb%d, layer_idx%d, failed to ion_import_dma_buf, shared_fd=%d!\n",
						hisifd->index, i, layer->img.shared_fd);
				} else {
					if (layer->img.mmu_enable == 1) {
						memset(&iommu_format, 0, sizeof(struct iommu_map_format));
						if (ion_map_iommu(hisifd->ion_client, ionhnd, &iommu_format)) {
							HISI_FB_ERR("fb%d, layer_idx%d, failed to ion_map_iommu, shared_fd=%d!\n",
								hisifd->index, i, layer->img.shared_fd);
						} else {
							has_map_iommu = true;
						}
					}

					add_tail = true;
				}
			}

			if (add_tail) {
				node = kzalloc(sizeof(struct hisifb_layerbuf), GFP_KERNEL);
				if (node == NULL) {
					HISI_FB_ERR("fb%d, layer_idx%d, failed to kzalloc!\n",
						hisifd->index, layer->layer_idx);

					if (ionhnd) {
						ion_free(hisifd->ion_client, ionhnd);
						ionhnd = NULL;
					}

					continue;
				}

				node->shared_fd = layer->img.shared_fd;
				node->frame_no = pov_req->frame_no;
				node->ion_handle = ionhnd;
				node->has_map_iommu = has_map_iommu;
				node->timeline = 0;
				//mmbuf
				node->mmbuf.addr = layer->img.mmbuf_base;
				node->mmbuf.size = layer->img.mmbuf_size;

				node->vir_addr = layer->img.vir_addr;
				node->chn_idx = layer->chn_idx;

				list_add_tail(&node->list_node, plock_list);

				if (g_debug_layerbuf_sync) {
					HISI_FB_INFO("fb%d, frame_no=%d, layer_idx(%d), shared_fd=%d, ion_handle=%pK, "
						"has_map_iommu=%d, timeline=%d, mmbuf(0x%x, %d).\n",
						hisifd->index, node->frame_no, i, node->shared_fd, node->ion_handle,
						node->has_map_iommu, node->timeline,
						node->mmbuf.addr, node->mmbuf.size);
				}
			}
		}
	}

	return 0;
}

void hisifb_layerbuf_flush(struct hisi_fb_data_type *hisifd,
	struct list_head *plock_list)
{
	struct hisifb_layerbuf *node, *_node_;
	unsigned long flags = 0;

	if (NULL == hisifd) {
		HISI_FB_ERR("hisifd is NULL");
		return;
	}
	if (NULL == hisifd->ion_client) {
		HISI_FB_ERR("ion_client is NULL");
		return;
	}
	if (NULL == plock_list) {
		HISI_FB_ERR("plock_list is NULL");
		return;
	}

	spin_lock_irqsave(&(hisifd->buf_sync_ctrl.layerbuf_spinlock), flags);
	hisifd->buf_sync_ctrl.layerbuf_flushed = true;
	list_for_each_entry_safe(node, _node_, plock_list, list_node) {
		list_del(&node->list_node);
		list_add_tail(&node->list_node, &(hisifd->buf_sync_ctrl.layerbuf_list));
	}
	spin_unlock_irqrestore(&(hisifd->buf_sync_ctrl.layerbuf_spinlock), flags);
}

void hisifb_layerbuf_unlock(struct hisi_fb_data_type *hisifd,
	struct list_head *pfree_list)
{
	struct hisifb_layerbuf *node, *_node_;

	if (NULL == hisifd) {
		HISI_FB_ERR("hisifd is NULL");
		return;
	}
	if (NULL == hisifd->ion_client) {
		HISI_FB_ERR("hisifd->ion_client is NULL");
		return;
	}
	if (NULL == hisifd->mmbuf_gen_pool) {
		HISI_FB_ERR("hisifd->mmbuf_gen_pool is NULL");
		return;
	}
	if (NULL == pfree_list) {
		HISI_FB_ERR("pfree_list is NULL");
		return;
	}

	list_for_each_entry_safe(node, _node_, pfree_list, list_node) {
		list_del(&node->list_node);

		if (g_debug_layerbuf_sync) {
			HISI_FB_INFO("fb%d, frame_no=%d, share_fd=%d, ion_handle=%pK, has_map_iommu=%d, "
				"timeline=%d, mmbuf(0x%x, %d).vir_addr = 0x%llx, chn_idx = %d\n",
				hisifd->index, node->frame_no, node->shared_fd, node->ion_handle, node->has_map_iommu,
				node->timeline, node->mmbuf.addr, node->mmbuf.size, node->vir_addr, node->chn_idx);
		}

		node->timeline = 0;
		if (node->ion_handle) {
			if (node->has_map_iommu) {
				ion_unmap_iommu(hisifd->ion_client, node->ion_handle);
			}
			ion_free(hisifd->ion_client, node->ion_handle);
		}
		kfree(node);
	}
}

void hisifb_layerbuf_lock_exception(struct hisi_fb_data_type *hisifd,
	struct list_head *plock_list)
{
	unsigned long flags = 0;

	if (NULL == hisifd) {
		HISI_FB_ERR("hisifd is NULL");
		return;
	}
	if (NULL == plock_list) {
		HISI_FB_ERR("plock_list is NULL");
		return;
	}

	spin_lock_irqsave(&(hisifd->buf_sync_ctrl.layerbuf_spinlock), flags);
	hisifd->buf_sync_ctrl.layerbuf_flushed = false;
	spin_unlock_irqrestore(&(hisifd->buf_sync_ctrl.layerbuf_spinlock), flags);

	hisifb_layerbuf_unlock(hisifd, plock_list);
}

static void hisifb_layerbuf_unlock_work(struct work_struct *work)
{
	struct hisifb_buf_sync *pbuf_sync = NULL;
	struct hisi_fb_data_type *hisifd = NULL;
	unsigned long flags;
	struct hisifb_layerbuf *node, *_node_;
	struct list_head free_list;

	pbuf_sync = container_of(work, struct hisifb_buf_sync, free_layerbuf_work);
	if (NULL == pbuf_sync) {
		HISI_FB_ERR("pbuf_sync is NULL");
		return;
	}
	hisifd = container_of(pbuf_sync, struct hisi_fb_data_type, buf_sync_ctrl);
	if (NULL == hisifd) {
		HISI_FB_ERR("hisifd is NULL");
		return;
	}
	if (NULL == hisifd->ion_client) {
		HISI_FB_ERR("hisifd->ion_client is NULL");
		return;
	}

	INIT_LIST_HEAD(&free_list);
	down(&pbuf_sync->layerbuf_sem);
	spin_lock_irqsave(&pbuf_sync->layerbuf_spinlock, flags);
	list_for_each_entry_safe(node, _node_, &pbuf_sync->layerbuf_list, list_node) {
		if (node->timeline >= 2) {
			list_del(&node->list_node);
			list_add_tail(&node->list_node, &free_list);
		}
	}
	spin_unlock_irqrestore(&pbuf_sync->layerbuf_spinlock, flags);
	up(&pbuf_sync->layerbuf_sem);
	hisifb_layerbuf_unlock(hisifd, &free_list);
}

/*lint -e429*/
int hisifb_offline_layerbuf_lock(struct hisi_fb_data_type *hisifd,
	dss_overlay_t *pov_req, struct list_head *plock_list)
{
	dss_overlay_block_t *pov_h_block_infos = NULL;
	dss_overlay_block_t *pov_h_block = NULL;
	dss_layer_t *layer = NULL;
	dss_wb_layer_t *wb_layer4block = NULL;
	uint32_t i = 0;
	uint32_t m = 0;
	struct hisifb_layerbuf *node = NULL; //lint !e429
	struct ion_handle *ionhnd = NULL;
	struct iommu_map_format iommu_format;
	bool add_tail = false;
	bool has_map_iommu = false;
	bool parameter_valid;

	parameter_valid = hisi_check_parameter_valid(hisifd, pov_req, plock_list);
	if (parameter_valid == false) {
		return -EINVAL;
	}

	wb_layer4block = &(pov_req->wb_layer_infos[0]);
	if (wb_layer4block->dst.shared_fd >= 0) {
		#if LINUX_VERSION_CODE >= KERNEL_VERSION(4,9,0)
			ionhnd = ion_import_dma_buf_fd(hisifd->ion_client, wb_layer4block->dst.shared_fd);
		#else
			ionhnd = ion_import_dma_buf(hisifd->ion_client, wb_layer4block->dst.shared_fd);
		#endif
			if (IS_ERR(ionhnd)) {
				ionhnd = NULL;
				HISI_FB_ERR("fb%d, wb_layer4block failed to ion_import_dma_buf, shared_fd=%d!\n",
					hisifd->index, wb_layer4block->dst.shared_fd);
			} else {
				if (wb_layer4block->dst.mmu_enable == 1) {
					memset(&iommu_format, 0, sizeof(struct iommu_map_format));
					if (ion_map_iommu(hisifd->ion_client, ionhnd, &iommu_format)) {
						HISI_FB_ERR("fb%d, wb_layer4block failed to ion_import_dma_buf, shared_fd=%d!\n",
							hisifd->index, wb_layer4block->dst.shared_fd);
					} else {
						has_map_iommu = true;
					}
				}
				add_tail = true;
			}
	}

	if (add_tail) {
		node = kzalloc(sizeof(struct hisifb_layerbuf), GFP_KERNEL);
		if (node == NULL) {
			if (ionhnd) {
				if (has_map_iommu) {
					ion_unmap_iommu(hisifd->ion_client, ionhnd);
					has_map_iommu = false;
				}
				ion_free(hisifd->ion_client, ionhnd);
				ionhnd = NULL;
			}
			HISI_FB_ERR("fb%d, wb_layer4block failed to kzalloc!\n", hisifd->index);
			return -EINVAL;
		}

		node->shared_fd = wb_layer4block->dst.shared_fd;
		node->ion_handle = ionhnd;
		node->has_map_iommu = has_map_iommu;

		//mmbuf
		node->mmbuf.addr = wb_layer4block->dst.mmbuf_base;
		node->mmbuf.size = wb_layer4block->dst.mmbuf_size;

		node->vir_addr = wb_layer4block->dst.vir_addr;
		node->chn_idx = wb_layer4block->chn_idx;

		list_add_tail(&node->list_node, plock_list);
		if (g_debug_offline_layerbuf_sync) {
			HISI_FB_INFO("fb%d, shared_fd=%d, ion_handle=%pK, has_map_iommu=%d, mmbuf(0x%x, %d).\n",
				hisifd->index, node->shared_fd, node->ion_handle,
				node->has_map_iommu, node->mmbuf.addr, node->mmbuf.size);
		}
	}

	pov_h_block_infos = (dss_overlay_block_t *)(pov_req->ov_block_infos_ptr);
	for (m = 0; m < pov_req->ov_block_nums; m++) {
		pov_h_block = &(pov_h_block_infos[m]);

		for (i = 0; i < pov_h_block->layer_nums; i++) {
			layer = &(pov_h_block->layer_infos[i]);
			add_tail = false;
			ionhnd = NULL;
			has_map_iommu = false;

			if (layer->dst_rect.y < pov_h_block->ov_block_rect.y)
				continue;

			if (layer->img.shared_fd < 0)
				continue;

			if ((layer->img.phy_addr == 0) &&
				(layer->img.vir_addr == 0) &&
				(layer->img.afbc_payload_addr == 0)) {
				HISI_FB_ERR("fb%d, layer_idx%d, chn_idx%d, no buffer!\n",
					hisifd->index, layer->layer_idx, layer->chn_idx);
				continue;
			}

			if (layer->img.shared_fd >= 0) {
			#if LINUX_VERSION_CODE >= KERNEL_VERSION(4,9,0)
				ionhnd = ion_import_dma_buf_fd(hisifd->ion_client, layer->img.shared_fd);
			#else
				ionhnd = ion_import_dma_buf(hisifd->ion_client, layer->img.shared_fd);
			#endif
				if (IS_ERR(ionhnd)) {
					ionhnd = NULL;
					HISI_FB_ERR("fb%d, layer_idx%d, failed to ion_import_dma_buf, shared_fd=%d!\n",
						hisifd->index, i, layer->img.shared_fd);
				} else {
					if (layer->img.mmu_enable == 1) {
						memset(&iommu_format, 0, sizeof(struct iommu_map_format));
						if (ion_map_iommu(hisifd->ion_client, ionhnd, &iommu_format)) {
							HISI_FB_ERR("fb%d, layer_idx%d, failed to ion_map_iommu, shared_fd=%d!\n",
								hisifd->index, i, layer->img.shared_fd);
						} else {
							has_map_iommu = true;
						}
					}
					add_tail = true;
				}
			}

			if (add_tail) {
				node = kzalloc(sizeof(struct hisifb_layerbuf), GFP_KERNEL); //lint !e423
				if (node == NULL) {
					if (ionhnd) {
						if (has_map_iommu) {
							ion_unmap_iommu(hisifd->ion_client, ionhnd);
							has_map_iommu = false;
						}
						ion_free(hisifd->ion_client, ionhnd);
						ionhnd = NULL;
					}
					HISI_FB_ERR("fb%d, layer_idx%d, failed to kzalloc!\n",
						hisifd->index, layer->layer_idx);
					continue;
				}

				node->shared_fd = layer->img.shared_fd;
				node->ion_handle = ionhnd;
				node->has_map_iommu = has_map_iommu;

				//mmbuf
				node->mmbuf.addr = layer->img.mmbuf_base;
				node->mmbuf.size = layer->img.mmbuf_size;

				node->vir_addr = layer->img.vir_addr;
				node->chn_idx = layer->chn_idx;

				list_add_tail(&node->list_node, plock_list);

				if (g_debug_offline_layerbuf_sync) {
					HISI_FB_INFO("fb%d, shared_fd=%d, ion_handle=%pK, has_map_iommu=%d, mmbuf(0x%x, %d).\n",
						hisifd->index, node->shared_fd, node->ion_handle,
						node->has_map_iommu, node->mmbuf.addr, node->mmbuf.size);
				}
			}
		}
	}

	return 0;
}
/*lint +e429*/
void hisifb_offline_layerbuf_unlock(struct hisi_fb_data_type *hisifd,
	struct list_head *pfree_list)
{
	struct hisifb_layerbuf *node, *_node_;

	if (NULL == hisifd) {
		HISI_FB_ERR("hisifd is NULL");
		return;
	}
	if (NULL == hisifd->ion_client) {
		HISI_FB_ERR("hisifd->ion_client is NULL");
		return;
	}
	if (NULL == pfree_list) {
		HISI_FB_ERR("pfree_list is NULL");
		return;
	}

	list_for_each_entry_safe(node, _node_, pfree_list, list_node) {
		list_del(&node->list_node);
		if (g_debug_offline_layerbuf_sync) {
			HISI_FB_INFO("fb%d, share_fd=%d, ion_handle=%pK, has_map_iommu=%d, "
				"mmbuf(0x%x, %d).vir_addr = 0x%llx, chn_idx = %d\n",
				hisifd->index, node->shared_fd, node->ion_handle, node->has_map_iommu,
				node->mmbuf.addr, node->mmbuf.size, node->vir_addr, node->chn_idx);
		}
		if (node->ion_handle) {
			if (node->has_map_iommu) {
				ion_unmap_iommu(hisifd->ion_client, node->ion_handle);
			}
			ion_free(hisifd->ion_client, node->ion_handle);
		}
		kfree(node);
	}
}

////////////////////////////////////////////////////////////////////////////////
//
// buf sync fence
//
#define BUF_SYNC_TIMEOUT_MSEC	(10 * MSEC_PER_SEC)

/**
 * hisi_dss_fb_sync_get_fence() - get fence from timeline
 * @timeline:	Timeline to create the fence on
 * @fence_name:	Name of the fence that will be created for debugging
 * @val:	Timeline value at which the fence will be signaled
 *
 * Function returns a fence on the timeline given with the name provided.
 * The fence created will be signaled when the timeline is advanced.
 */
static struct hisi_dss_fence *hisi_dss_fb_sync_get_fence(struct hisi_dss_timeline *timeline,
		const char *fence_name, int val)
{
	struct hisi_dss_fence *fence;

	fence = hisi_dss_get_sync_fence(timeline, fence_name, NULL, val);
	if (fence == NULL) {
		HISI_FB_ERR("%s: cannot create fence\n", fence_name);
		return NULL;
	}

	return fence;
}

static struct hisi_dss_fence *hisifb_buf_create_fence(struct hisi_fb_data_type *hisifb,
	struct hisifb_buf_sync *buf_sync_ctrl, u32 fence_type,
	int *fence_fd, int value)
{
	struct hisi_dss_fence *sync_fence = NULL;
	char fence_name[32];

	if (fence_type == HISI_DSS_RETIRE_FENCE) {
		snprintf(fence_name, sizeof(fence_name), "fb%d_retire", hisifb->index);
		sync_fence = hisi_dss_fb_sync_get_fence(
					buf_sync_ctrl->timeline_retire,
					fence_name, value);
	} else {
		snprintf(fence_name, sizeof(fence_name), "fb%d_release", hisifb->index);
		sync_fence = hisi_dss_fb_sync_get_fence(
					buf_sync_ctrl->timeline,
					fence_name, value);
	}

	if (IS_ERR_OR_NULL(sync_fence)) {
		HISI_FB_ERR("%s: unable to retrieve release fence\n", fence_name);
		goto end;
	}

	/* get fence fd */
	*fence_fd = hisi_dss_get_sync_fence_fd(sync_fence);
	if (*fence_fd < 0) {
		HISI_FB_ERR("%s: get_unused_fd_flags failed error:0x%x\n",
			fence_name, *fence_fd);
		hisi_dss_put_sync_fence(sync_fence);
		sync_fence = NULL;
		goto end;
	}

end:
	return sync_fence;
}

int hisifb_buf_sync_create_fence(struct hisi_fb_data_type *hisifd, dss_overlay_t *pov_req)
{
	int value, ret = 0;
	struct hisi_dss_fence *release_fence, *retire_fence;
	struct hisifb_buf_sync *buf_sync_ctrl = NULL;

	if (!hisifd) {
		HISI_FB_ERR("hisifd is NULL!\n");
		return -EINVAL;
	}

	if (!pov_req) {
		HISI_FB_ERR("pov_req is NULL!\n");
		return -EINVAL;
	}

	if (hisifd->index != PRIMARY_PANEL_IDX) {
		pov_req->release_fence = -1;
		pov_req->retire_fence = -1;
		return 0;
	}

	buf_sync_ctrl = &hisifd->buf_sync_ctrl;
	value = buf_sync_ctrl->timeline_max + buf_sync_ctrl->threshold + 1;
	release_fence = hisifb_buf_create_fence(hisifd, buf_sync_ctrl,
		HISI_DSS_RELEASE_FENCE, &pov_req->release_fence, value);
	if (IS_ERR_OR_NULL(release_fence)) {
		HISI_FB_ERR("unable to retrieve release fence\n");
		ret = PTR_ERR(release_fence);
		goto release_fence_err;
	}

	value += buf_sync_ctrl->retire_threshold;
	retire_fence = hisifb_buf_create_fence(hisifd, buf_sync_ctrl,
		HISI_DSS_RETIRE_FENCE, &pov_req->retire_fence, value);
	if (IS_ERR_OR_NULL(retire_fence)) {
		HISI_FB_ERR("unable to retrieve retire fence\n");
		ret = PTR_ERR(retire_fence);
		goto retire_fence_err;
	}

	if (g_debug_fence_timeline) {
		HISI_FB_INFO("hisifb%d frame_no(%d) create fence timeline_max(%d), %s(%d),"
			"%s(%d), timeline(%d), timeline_retire(%d)!\n",
			hisifd->index, hisifd->ov_req.frame_no,
			buf_sync_ctrl->timeline_max,
			release_fence->name, release_fence->base.seqno,
			retire_fence->name, retire_fence->base.seqno,
			buf_sync_ctrl->timeline->value,
			buf_sync_ctrl->timeline_retire->value);
	}

	return ret;

retire_fence_err:
	put_unused_fd(pov_req->release_fence);
	hisi_dss_put_sync_fence(release_fence);
release_fence_err:
	pov_req->retire_fence = -1;
	pov_req->release_fence = -1;

	return ret;
}

int hisifb_buf_sync_wait(int fence_fd)
{
	int ret = 0;
	struct hisi_dss_fence *fence = NULL;

	fence = hisi_dss_get_fd_sync_fence(fence_fd);
	if (!fence){
		HISI_FB_ERR("fence_fd=%d, sync_fence_fdget failed!\n", fence_fd);
		return -EINVAL;
	}

	ret = hisi_dss_wait_sync_fence(fence, BUF_SYNC_TIMEOUT_MSEC);
	if (ret < 0) {
		HISI_FB_ERR("Waiting on fence failed, fence_fd: %d, ret: %d.\n", fence_fd, ret);
	}

	hisi_dss_put_sync_fence(fence);

	return ret;
}

void hisifb_buf_sync_suspend(struct hisi_fb_data_type *hisifd)
{
	struct hisifb_buf_sync *buf_sync_ctrl = NULL;
	unsigned long flags;
	int val = 0;

	if (NULL == hisifd) {
		HISI_FB_ERR("hisifd is NULL");
		return;
	}

	buf_sync_ctrl = &hisifd->buf_sync_ctrl;
	if (NULL == buf_sync_ctrl->timeline) {
		HISI_FB_ERR("timeline NULL Pointer!\n");
		return;
	}

	spin_lock_irqsave(&buf_sync_ctrl->refresh_lock, flags);
	if ((buf_sync_ctrl->timeline->next_value - buf_sync_ctrl->timeline->value) > 0) {
		val = buf_sync_ctrl->timeline->next_value - buf_sync_ctrl->timeline->value;
	}

	hisi_dss_resync_timeline(buf_sync_ctrl->timeline);
	hisi_dss_resync_timeline(buf_sync_ctrl->timeline_retire);

	buf_sync_ctrl->timeline_max += val;
	buf_sync_ctrl->refresh = 0;

	spin_unlock_irqrestore(&buf_sync_ctrl->refresh_lock, flags);
	if (g_debug_fence_timeline) {
		HISI_FB_INFO("fb%d frame_no(%d) timeline_max(%d), TL(Nxt %d , Crnt %d)!\n",
			hisifd->index, hisifd->ov_req.frame_no, buf_sync_ctrl->timeline_max,
			buf_sync_ctrl->timeline->next_value, buf_sync_ctrl->timeline->value);
	}
}


void hisifb_buf_sync_close_fence(dss_overlay_t *pov_req)
{
	if (!pov_req) {
		HISI_FB_ERR("pov_req is NULL!\n");
		return;
	}

	if (pov_req->release_fence >= 0) {
		sys_close(pov_req->release_fence);
		pov_req->release_fence = -1;
	}

	if (pov_req->retire_fence >= 0) {
		sys_close(pov_req->retire_fence);
		pov_req->retire_fence = -1;
	}
}

int hisifb_buf_sync_handle_offline(struct hisi_fb_data_type *hisifd, dss_overlay_t *pov_req)
{
	dss_overlay_block_t *pov_h_block_infos = NULL;
	dss_overlay_block_t *pov_h_block = NULL;
	dss_layer_t *layer = NULL;
	dss_wb_layer_t *wb_layer = NULL;
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

	if (pov_req->wb_enable) {
		for (i = 0; i < pov_req->wb_layer_nums; i++) {
			wb_layer = &(pov_req->wb_layer_infos[i]);

			if (wb_layer->acquire_fence >= 0) {
				hisifb_buf_sync_wait(wb_layer->acquire_fence);
			}
		}
	}

	pov_h_block_infos = (dss_overlay_block_t *)(pov_req->ov_block_infos_ptr);
	for (m = 0; m < pov_req->ov_block_nums; m++) {
		pov_h_block = &(pov_h_block_infos[m]);

		for (i = 0; i < pov_h_block->layer_nums; i++) {
			layer = &(pov_h_block->layer_infos[i]);

			if (layer->dst_rect.y < pov_h_block->ov_block_rect.y)
				continue;

			if (layer->acquire_fence >= 0) {
				hisifb_buf_sync_wait(layer->acquire_fence);
			}
		}
	}

	return 0;
}

int hisifb_buf_sync_handle(struct hisi_fb_data_type *hisifd, dss_overlay_t *pov_req)
{
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

	pov_h_block_infos = (dss_overlay_block_t *)(pov_req->ov_block_infos_ptr);
	for (m = 0; m < pov_req->ov_block_nums; m++) {
		pov_h_block = &(pov_h_block_infos[m]);

		for (i = 0; i < pov_h_block->layer_nums; i++) {
			layer = &(pov_h_block->layer_infos[i]);

			if (layer->dst_rect.y < pov_h_block->ov_block_rect.y)
				continue;

			if (layer->acquire_fence >= 0) {
				hisifb_buf_sync_wait(layer->acquire_fence);
			}
		}
	}

	return 0;
}

void hisifb_buf_sync_signal(struct hisi_fb_data_type *hisifd)
{
	int val = 0;
	struct hisifb_layerbuf *node = NULL;
	struct hisifb_layerbuf *_node_ = NULL;
	struct hisifb_buf_sync *buf_sync_ctrl = NULL;

	if (NULL == hisifd) {
		HISI_FB_ERR("hisifd is NULL");
		return;
	}
	buf_sync_ctrl = &hisifd->buf_sync_ctrl;

	spin_lock(&buf_sync_ctrl->refresh_lock);
	if (buf_sync_ctrl->refresh) {
		val = buf_sync_ctrl->refresh;
		hisi_dss_inc_timeline(buf_sync_ctrl->timeline, val);
		hisi_dss_inc_timeline(buf_sync_ctrl->timeline_retire, val);

		buf_sync_ctrl->timeline_max += val;
		buf_sync_ctrl->refresh = 0;

		if (g_debug_fence_timeline) {
			HISI_FB_INFO("hisifb%d frame_no(%d) timeline_max(%d), timeline(%d), timeline_retire(%d)!\n",
				hisifd->index, hisifd->ov_req.frame_no,
				buf_sync_ctrl->timeline_max,
				buf_sync_ctrl->timeline->value,
				buf_sync_ctrl->timeline_retire->value);
		}
	}
	spin_unlock(&buf_sync_ctrl->refresh_lock);

	spin_lock(&(buf_sync_ctrl->layerbuf_spinlock));
	list_for_each_entry_safe(node, _node_, &(buf_sync_ctrl->layerbuf_list), list_node) {
		if (buf_sync_ctrl->layerbuf_flushed) {
			node->timeline++;
		}
	}
	buf_sync_ctrl->layerbuf_flushed = false;
	spin_unlock(&(buf_sync_ctrl->layerbuf_spinlock));

	queue_work(buf_sync_ctrl->free_layerbuf_queue, &(buf_sync_ctrl->free_layerbuf_work));
}

void hisifb_buf_sync_register(struct platform_device *pdev)
{
	struct hisi_fb_data_type *hisifd = NULL;
	char tmp_name[256] = {0};
	struct hisifb_buf_sync *buf_sync_ctrl = NULL;

	if (NULL == pdev) {
		HISI_FB_ERR("pdev is NULL");
		return;
	}
	hisifd = platform_get_drvdata(pdev);
	if (NULL == hisifd) {
		dev_err(&pdev->dev, "hisifd is NULL");
		return;
	}

	HISI_FB_DEBUG("fb%d, +.\n", hisifd->index);
	buf_sync_ctrl = &hisifd->buf_sync_ctrl;

	buf_sync_ctrl->fence_name = "dss-fence";

	buf_sync_ctrl->threshold = 0;
	buf_sync_ctrl->retire_threshold = 0;

	buf_sync_ctrl->refresh = 0;
	buf_sync_ctrl->timeline_max = 1;
	spin_lock_init(&buf_sync_ctrl->refresh_lock);

	snprintf(tmp_name, sizeof(tmp_name), "hisi_dss_fb%d", hisifd->index);
	buf_sync_ctrl->timeline = hisi_dss_create_timeline(tmp_name);
	if (buf_sync_ctrl->timeline == NULL) {
		HISI_FB_ERR("cannot create release fence time line\n");
		return ; /* -ENOMEM */
	}

	snprintf(tmp_name, sizeof(tmp_name), "hisi_dss_fb%d_retire", hisifd->index);
	buf_sync_ctrl->timeline_retire = hisi_dss_create_timeline(tmp_name);
	if (buf_sync_ctrl->timeline_retire == NULL) {
		HISI_FB_ERR("cannot create retire fence time line\n");
		return ; /* -ENOMEM */
	}

	// handle free layerbuf
	spin_lock_init(&(buf_sync_ctrl->layerbuf_spinlock));
	INIT_LIST_HEAD(&(buf_sync_ctrl->layerbuf_list));
	buf_sync_ctrl->layerbuf_flushed = false;
	sema_init(&(buf_sync_ctrl->layerbuf_sem), 1);

	snprintf(tmp_name, sizeof(tmp_name), HISI_DSS_LAYERBUF_FREE, hisifd->index);
	INIT_WORK(&(buf_sync_ctrl->free_layerbuf_work), hisifb_layerbuf_unlock_work);
	buf_sync_ctrl->free_layerbuf_queue = create_singlethread_workqueue(tmp_name);
	if (!buf_sync_ctrl->free_layerbuf_queue) {
		dev_err(&pdev->dev, "failed to create free_layerbuf_queue!\n");
		return ;
	}

	HISI_FB_DEBUG("fb%d, -.\n", hisifd->index);
}

void hisifb_buf_sync_unregister(struct platform_device *pdev)
{
	struct hisi_fb_data_type *hisifd = NULL;
	struct hisifb_buf_sync *buf_sync_ctrl = NULL;

	if (NULL == pdev) {
		HISI_FB_ERR("pdev is NULL");
		return;
	}
	hisifd = platform_get_drvdata(pdev);
	if (NULL == hisifd) {
		dev_err(&pdev->dev, "hisifd is NULL");
		return;
	}

	HISI_FB_DEBUG("fb%d, +.\n", hisifd->index);
	buf_sync_ctrl = &hisifd->buf_sync_ctrl;

	if (buf_sync_ctrl->timeline) {
		hisi_dss_destroy_timeline(buf_sync_ctrl->timeline);
		buf_sync_ctrl->timeline = NULL;
	}

	if (buf_sync_ctrl->timeline_retire) {
		hisi_dss_destroy_timeline(buf_sync_ctrl->timeline_retire);
		buf_sync_ctrl->timeline_retire = NULL;
	}

	if (buf_sync_ctrl->free_layerbuf_queue) {
		destroy_workqueue(buf_sync_ctrl->free_layerbuf_queue);
		buf_sync_ctrl->free_layerbuf_queue = NULL;
	}

	HISI_FB_DEBUG("fb%d, -.\n", hisifd->index);
}
