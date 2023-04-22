/*
 * jpeg_base.h
 *
 * implement for jpeg subsystem.
 *
 * Copyright (c) 2001-2021, Huawei Tech. Co., Ltd. All rights reserved.
 *
 * luolidong <luolidong@huawei.com>
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

#ifndef __HW_JPEG_BASE_H__
#define __HW_JPEG_BASE_H__

#include <linux/clk.h>

extern int jpeg_enc_set_rate(struct clk *clk, unsigned long rate);
extern int jpeg_enc_clk_prepare_enable(struct clk *clk);
extern void jpeg_enc_clk_disable_unprepare(struct clk *clk);

extern int jpeg_dec_set_rate(struct clk *clk, unsigned long rate);
extern int jpeg_dec_clk_prepare_enable(struct clk *clk);
extern void jpeg_dec_clk_disable_unprepare(struct clk *clk);

extern int jpeg_ipp_set_rate(struct clk *clk, unsigned long rate);	
extern int jpeg_ipp_clk_prepare_enable(struct clk *clk);	
extern void jpeg_ipp_clk_disable_unprepare(struct clk *clk);

#endif /* __HW_JPEG_BASE_H__ */
