/*
 *
 * Copyright (C) 2011 Google, Inc.
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */

#include <linux/kernel.h>
#include <linux/file.h>
#include <linux/fs.h>
#include <linux/uaccess.h>

#include "ion.h"
#include "ion_priv.h"
#include "compat_ion.h"
#ifdef CONFIG_HW_FDLEAK
#include <chipset_common/hwfdleak/fdleak.h>
#endif

union ion_ioctl_arg {
	struct ion_fd_data fd;
	struct ion_allocation_data allocation;
	struct ion_handle_data handle;
	struct ion_custom_data custom;
	struct ion_heap_query query;
	struct ion_map_iommu_data map_iommu;
};

static int validate_ioctl_arg(unsigned int cmd, union ion_ioctl_arg *arg)
{
	int ret = 0;

	switch (cmd) {
	case ION_IOC_HEAP_QUERY:
		ret = arg->query.reserved0 != 0;
		ret |= arg->query.reserved1 != 0; /*lint !e514*/
		ret |= arg->query.reserved2 != 0; /*lint !e514*/
		break;
	default:
		break;
	}

	return ret ? -EINVAL : 0;
}

/* fix up the cases where the ioctl direction bits are incorrect */
static unsigned int ion_ioctl_dir(unsigned int cmd)
{
	switch (cmd) {
	case ION_IOC_SYNC:
	case ION_IOC_FREE:
	case ION_IOC_CUSTOM:
		return _IOC_WRITE;
	default:
		return _IOC_DIR(cmd);
	}
}

long ion_ioctl(struct file *filp, unsigned int cmd, unsigned long arg)
{
	struct ion_client *client = filp->private_data;
	struct ion_device *dev = client->dev;
	struct ion_handle *cleanup_handle = NULL;
	int ret = 0;
	unsigned int dir;
	union ion_ioctl_arg data;

	dir = ion_ioctl_dir(cmd);

	if (_IOC_SIZE(cmd) > sizeof(data)) {
		pr_err("%s: cmd size too large!\n", __func__);
 		return -EINVAL;
	}

	/*
	 * The copy_from_user is unconditional here for both read and write
	 * to do the validate. If there is no write for the ioctl, the
	 * buffer is cleared
	 */
	if (copy_from_user(&data, (void __user *)arg, _IOC_SIZE(cmd))){
		pr_err("%s: copy arg failed!\n", __func__);
		return -EFAULT;
    }

	ret = validate_ioctl_arg(cmd, &data);
	if (ret) {
		pr_warn_once("%s: ioctl validate failed\n", __func__);
		return ret;
	}

	if (!(dir & _IOC_WRITE))
		memset(&data, 0, sizeof(data));

	switch (cmd) {
	case ION_IOC_ALLOC:
	{
		struct ion_handle *handle;

		handle = __ion_alloc(client, data.allocation.len,
						data.allocation.align,
						data.allocation.heap_id_mask,
						data.allocation.flags, true);
		if (IS_ERR(handle)) {
			pr_err("%s: ion alloc failed!\n", __func__);
			pr_err("len:%lx,align:%lx,heap_id_mask:%x,flags:%x\n",
				data.allocation.len,
				data.allocation.align,
				data.allocation.heap_id_mask,
				data.allocation.flags);
			return PTR_ERR(handle);
		}

		data.allocation.handle = handle->id;

		cleanup_handle = handle;
		break;
	}
	case ION_IOC_FREE:
	{
		struct ion_handle *handle;

		mutex_lock(&client->lock);
		handle = ion_handle_get_by_id_nolock(client, data.handle.handle);
		if (IS_ERR(handle)) {
			mutex_unlock(&client->lock);
			return PTR_ERR(handle);
		}
		ion_free_nolock(client, handle);
		ion_handle_put_nolock(handle);
		mutex_unlock(&client->lock);
		break;
	}
	case ION_IOC_SHARE:
	case ION_IOC_MAP:
	{
		struct ion_handle *handle;

		handle = ion_handle_get_by_id(client, data.handle.handle);
		if (IS_ERR(handle)) {
			pr_err("handle is error %d\n", __LINE__);
			return PTR_ERR(handle);
		}

		data.fd.fd = ion_share_dma_buf_fd(client, handle);
		ion_handle_put(handle);
		if (data.fd.fd < 0)
			ret = data.fd.fd;
        #ifdef CONFIG_HW_FDLEAK
                if (ION_IOC_SHARE == cmd)
                        fdleak_report(FDLEAK_WP_DMABUF, 0);
                else
                        fdleak_report(FDLEAK_WP_DMABUF, 1);
        #endif
		break;
	}
	case ION_IOC_IMPORT:
	{
		struct ion_handle *handle;

		handle = ion_import_dma_buf_fd(client, data.fd.fd);
		if (IS_ERR(handle)) {
			pr_err("handle is error %d\n", __LINE__);
			ret = PTR_ERR(handle);
		}
		else
			data.handle.handle = handle->id;
        #ifdef CONFIG_HW_FDLEAK
                fdleak_report(FDLEAK_WP_DMABUF, 2);
        #endif
		break;
	}
	case ION_IOC_SYNC:
	{
		ret = ion_sync_for_device(client, data.fd.fd);
		break;
	}
	case ION_IOC_CUSTOM:
	{
		if (!dev->custom_ioctl) {
			pr_err("ion ENOTTY error %s %d\n", __func__, __LINE__);
			return -ENOTTY;
		}
		ret = dev->custom_ioctl(client, data.custom.cmd,
						data.custom.arg);
		break;
	}
	case ION_IOC_HEAP_QUERY:
		ret = ion_query_heaps(client, &data.query);
		break;

	case ION_IOC_INV:
	{
		ion_sync_for_cpu(client, data.fd.fd);
		break;
	}

	case ION_IOC_MAP_IOMMU:
	{
		struct ion_handle *handle;

		handle = ion_handle_get_by_id(client, data.map_iommu.handle);
		if (IS_ERR(handle)) {
			pr_err("%s: map iommu but handle invalid!\n", __func__);
			return PTR_ERR(handle);
		}

		ret = ion_map_iommu(client, handle, &data.map_iommu.format);

		ion_handle_put(handle);
		break;
	}
	case ION_IOC_UNMAP_IOMMU:
	{
		struct ion_handle *handle;

		handle = ion_handle_get_by_id(client, data.map_iommu.handle);
		if (IS_ERR(handle)) {
			pr_err("%s: map iommu but handle invalid!\n", __func__);
			return PTR_ERR(handle);
		}

		ion_unmap_iommu(client, handle);
		data.map_iommu.format.iova_start = 0;
		data.map_iommu.format.iova_size = 0;

		ion_handle_put(handle);
		break;
	}

	default:
		pr_err("ion ENOTTY error %s %d\n", __func__, __LINE__);
		return -ENOTTY;
	}

	if (dir & _IOC_READ) {
		if (copy_to_user((void __user *)arg, &data, _IOC_SIZE(cmd))) {
			pr_err("%s: copy to user failed! cmd: %d\n",
					__func__, cmd);
			if (cleanup_handle) {
				ion_free(client, cleanup_handle);
				ion_handle_put(cleanup_handle);
			}
			return -EFAULT;
		}
	}
	if (cleanup_handle)
		ion_handle_put(cleanup_handle);
	return ret;
}
