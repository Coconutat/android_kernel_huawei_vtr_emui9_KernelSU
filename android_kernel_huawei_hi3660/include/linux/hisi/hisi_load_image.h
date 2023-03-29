/*
 * hisi_load_image.h
 *
 * Copyright (c) 2001-2021, Huawei Tech. Co., Ltd. All rights reserved.
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
#ifndef __HISI_LOAD_IMAGE_H__
#define __HISI_LOAD_IMAGE_H__

/*****************************************************************************
 Struct Definition
*****************************************************************************/
struct load_image_info {
	unsigned int ecoretype;
	unsigned int image_size;
	unsigned long image_addr;
	char *partion_name;
};

/*****************************************************************************
 Function:     bsp_load_and_verify_image
 Description:  This function is used for image in nonsecure world load to
               secure world, include image reset/load/sec-verify/disreset.
 Parameters:   struct load_image_info *img_info
 Return:       SEC_OK:    success
               SEC_ERROR: failure
 Instuctions:  1. Fill the load_image_info struct, note the following points:
               ecoretype is soc type should match the definition in secos.
               image_addr should align with cacheline.
               image_size represent load image size.
               partion_name should be consistent with ptable.
               2. Call bsp_load_and_verify_image and check return value after
               filled the load_image_info struct.
*****************************************************************************/
int bsp_load_and_verify_image(struct load_image_info *img_info);

int bsp_load_sec_img(struct load_image_info *img_info);

#endif /* end of hisi_load_image.h */
