 /* Copyright (c) 2013-2014, Hisilicon Tech. Co., Ltd. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 and
 * only version 2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */

#include "hisi_fb.h"


int hisi_ion_get_phys(struct fb_info *info, void __user *arg)
{
	struct ion_phys_data data;
	ion_phys_addr_t phys_addr = 0;
	size_t size = 0;
	int ret = 0;
	struct hisi_fb_data_type *hisifd = NULL;
	struct ion_handle *handle;


	hisifd = (struct hisi_fb_data_type *)info->par;
	if (NULL == hisifd || !(hisifd->pdev)) {
		HISI_FB_ERR("hisifd NULL Pointer");
		return -EFAULT;
	}

	if (copy_from_user(&data, (void __user *)arg,sizeof(data))) {
		return -EFAULT;
	}

	if (NULL == hisifd->ion_client) {
		HISI_FB_ERR("Failed to get the ion_client");
		return -EFAULT;
	}

#if LINUX_VERSION_CODE >= KERNEL_VERSION(4,9,0)
	handle = ion_import_dma_buf_fd(hisifd->ion_client, data.fd_buffer);
#else
	handle = ion_import_dma_buf(hisifd->ion_client, data.fd_buffer);
#endif
	if (IS_ERR(handle))
		return -EFAULT;

	ret = hisifb_ion_phys(hisifd->ion_client, handle, &(hisifd->pdev->dev), &phys_addr, &size);
	if (ret) {
		ion_free(hisifd->ion_client, handle);
		HISI_FB_ERR("hisifb_ion_phys:failed to get phys addr");
		return -EFAULT;
	}

	data.size = size & 0xffffffff;
	data.phys_l = phys_addr & 0xffffffff;
	data.phys_h = (phys_addr >> 32) & 0xffffffff;

	if (copy_to_user((void __user *)arg, &data, sizeof(data))) {
		ion_free(hisifd->ion_client, handle);
		return -EFAULT;
	}
	ion_free(hisifd->ion_client, handle);
	return 0;
}

int hisifb_ion_phys(struct ion_client *client, struct ion_handle *handle, struct device *dev,
                 ion_phys_addr_t *addr, size_t *len)
{
	int ret = 0;
	if (NULL == client || NULL == handle) {
		HISI_FB_ERR("hisifb_ion_phys NULL Pointer");
		return -EFAULT;
	}

#if LINUX_VERSION_CODE >= KERNEL_VERSION(4,9,0)
	struct sg_table *table = NULL;
	struct dma_buf *buf = NULL;
	struct dma_buf_attachment *attach = NULL;
	int shared_fd = 0;
	shared_fd = ion_share_dma_buf_fd(client, handle);
	if (shared_fd < 0) {
		HISI_FB_ERR("Failed to share ion buffer(0x%pK)!", handle);
		return -EFAULT;
	}

	buf = dma_buf_get(shared_fd);
	if (IS_ERR(buf)) {
		sys_close(shared_fd);
		HISI_FB_ERR("Invalid file handle(%d)", shared_fd);
		return -EFAULT;
	}

	attach = dma_buf_attach(buf, dev);
	if (IS_ERR(attach)) {
		dma_buf_put(buf);
		sys_close(shared_fd);
		return -EFAULT;
	}

	table = dma_buf_map_attachment(attach, DMA_BIDIRECTIONAL);
	if (IS_ERR(table)) {
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
	return 0;
#else
	ret = ion_phys(client, handle, addr, len);
	if (ret) {
		HISI_FB_ERR("failed to ion_phys");
		return -EFAULT;
	}
	return 0;
#endif
}

struct sg_table *hisifb_ion_sg_table(struct ion_client *client, struct ion_handle *handle,
        struct device *dev)
{

	struct sg_table *table = NULL;
	if (NULL == client || NULL == handle) {
		HISI_FB_ERR("hisifb_ion_sg_table NULL Pointer");
		return ERR_PTR(-EINVAL);
	}

#if LINUX_VERSION_CODE >= KERNEL_VERSION(4,9,0)
	struct dma_buf *buf = NULL;
	struct dma_buf_attachment *attach = NULL;
	int shared_fd = 0;
	shared_fd = ion_share_dma_buf_fd(client, handle);
	if (shared_fd < 0) {
		HISI_FB_ERR("Failed to share ion buffer(0x%pK)!", handle);
		return -EFAULT;
	}

	buf = dma_buf_get(shared_fd);
	if (IS_ERR(buf)) {
		sys_close(shared_fd);
		HISI_FB_ERR("Invalid file handle(%d)", shared_fd);
		return -EFAULT;
	}
	attach = dma_buf_attach(buf, dev);
	if (IS_ERR(attach)) {
		dma_buf_put(buf);
		sys_close(shared_fd);
		return -EFAULT;
	}
	table = dma_buf_map_attachment(attach, DMA_BIDIRECTIONAL);
	if (IS_ERR(table)) {
		dma_buf_detach(buf, attach);
		dma_buf_put(buf);
		sys_close(shared_fd);
		return -EFAULT;
    }
	dma_buf_unmap_attachment(attach, table, DMA_BIDIRECTIONAL);
	dma_buf_detach(buf, attach);
	dma_buf_put(buf);
	sys_close(shared_fd);
#else
	table = ion_sg_table(client, handle);
#endif
	return table;
}
