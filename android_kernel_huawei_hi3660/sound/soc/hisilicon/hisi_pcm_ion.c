/* Copyright (c) 2013-2018, Hisilicon Tech. Co., Ltd. All rights reserved.
*
* This program is free software; you can redistribute it and/or modify
* it under the terms of the GNU General Public License version 2 and
* only version 2 as published by the Free Software Foundation.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
* GNU General Public License for more details.
*/

#include <linux/err.h>
#include <linux/mutex.h>
#include <linux/dma-buf.h>
#include <linux/slab.h>
#include <linux/string.h>
#include <linux/version.h>
#include <linux/platform_device.h>
#include <linux/of_device.h>
#include <linux/syscalls.h>
#include <linux/hisi/hisi_ion.h>
#include "hisi_snd_log.h"
#include "hisi_pcm_ion.h"

int hisi_pcm_ion_phys(struct ion_client *client, struct ion_handle *handle,
	struct device *dev, ion_phys_addr_t *addr)
{
	struct sg_table *table = NULL;
	struct dma_buf *buf = NULL;
	struct dma_buf_attachment *attach = NULL;
	int shared_fd = 0;
	int ret = 0;

	if (client == NULL || handle == NULL) {
		loge("ion client or handle is null\n");
		return -EFAULT;
	}

	shared_fd = ion_share_dma_buf_fd(client, handle);
	if (shared_fd < 0) {
		loge("Failed to share ion buffer(0x%pK)!\n", handle);
		return -EFAULT;
	}

	buf = dma_buf_get(shared_fd);
	if (IS_ERR(buf)) {
		loge("Invalid file handle(%d)\n", shared_fd);
		sys_close(shared_fd);
		return -EFAULT;
	}

	attach = dma_buf_attach(buf, dev);
	if (IS_ERR(attach)) {
		dma_buf_put(buf);
		sys_close(shared_fd);
		return -EFAULT;
	}

	table = dma_buf_map_attachment(attach, DMA_BIDIRECTIONAL);
	if (IS_ERR_OR_NULL(table)) {
		dma_buf_detach(buf, attach);
		dma_buf_put(buf);
		sys_close(shared_fd);
		return -EFAULT;
	}

	*addr = sg_phys(table->sgl);

	dma_buf_unmap_attachment(attach, table, DMA_BIDIRECTIONAL);
	dma_buf_detach(buf, attach);
	dma_buf_put(buf);
	sys_close(shared_fd);

	return ret;
}

struct sg_table *hisi_pcm_ion_sg_table(struct ion_client *client, struct ion_handle *handle,
        struct device *dev)
{
	struct dma_buf *buf = NULL;
	struct dma_buf_attachment *attach = NULL;
	int shared_fd = 0;
	struct sg_table *table = NULL;

	if (NULL == client || NULL == handle) {
		loge("hisifb_ion_sg_table NULL Pointer\n");
		return ERR_PTR(-EINVAL);
	}

	shared_fd = ion_share_dma_buf_fd(client, handle);
	if (shared_fd < 0) {
		loge("Failed to share ion buffer(0x%pK)!\n", handle);
		return ERR_PTR(-EINVAL);
	}

	buf = dma_buf_get(shared_fd);
	if (IS_ERR(buf)) {
		loge("Invalid file handle(%d)\n", shared_fd);
		sys_close(shared_fd);
		return ERR_PTR(-EFAULT);
	}

	attach = dma_buf_attach(buf, dev);
	if (IS_ERR(attach))
		goto fail_attach;

	table = dma_buf_map_attachment(attach, DMA_BIDIRECTIONAL);
	if (IS_ERR(table)) {
		dma_buf_detach(buf, attach);
		goto fail_attach;
	}

	dma_buf_unmap_attachment(attach, table, DMA_BIDIRECTIONAL);
	dma_buf_detach(buf, attach);
	dma_buf_put(buf);

	sys_close(shared_fd);

	return table;

fail_attach:
	dma_buf_put(buf);
	sys_close(shared_fd);
	return ERR_PTR(-EFAULT);
}

