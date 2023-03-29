/*
 * hjpeg_intf.h
 *
 * provide Macro and struct for jpeg.
 *
 * Copyright (c) 2001-2021, Huawei Tech. Co., Ltd. All rights reserved.
 *
 * zhuchunyu@hisilicon.com
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */


typedef struct _tag_hjpeg_intf hjpeg_intf_t;

typedef struct _tag_hjpeg_vtbl {
    int (*get_name)(hjpeg_intf_t *i);
    int (*encode_process) (hjpeg_intf_t *i, void *cfg);
    int (*power_on) (hjpeg_intf_t *i);
    int (*power_down) (hjpeg_intf_t *i);
    int (*get_reg) (hjpeg_intf_t *i, void* cfg);
    int (*set_reg) (hjpeg_intf_t *i, void* cfg);
}hjpeg_vtbl_t;

typedef struct _tag_hjpeg_intf {
    hjpeg_vtbl_t *vtbl;
} hjpeg_intf_t;

extern int
hjpeg_register(
        struct platform_device* pdev,
        hjpeg_intf_t* si);

extern void
hjpeg_unregister(hjpeg_intf_t* si);


