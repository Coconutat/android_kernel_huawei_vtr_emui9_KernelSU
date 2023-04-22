/* Copyright (c) 2013-2018, Hisilicon Tech. Co., Ltd. All rights reserved.
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
#ifndef HISI_PCM_ION_H
#define HISI_PCM_ION_H

#include <linux/ion.h>

int hisi_pcm_ion_phys(struct ion_client *client, struct ion_handle *handle,
	struct device *dev, ion_phys_addr_t *addr);
struct sg_table *hisi_pcm_ion_sg_table(struct ion_client *client, struct ion_handle *handle,
        struct device *dev);
#endif
