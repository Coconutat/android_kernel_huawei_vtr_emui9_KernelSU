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

#ifndef __DPTX_DBG_H__
#define __DPTX_DBG_H__

/*#define DPTX_DEBUG_REG*/
/*#define DPTX_DEBUG_AUX*/
#define DPTX_DEBUG_IRQ
#define DPTX_DEBUG_DPCD_CMDS

#define dptx_dbg(_dp, _fmt...) dev_dbg(_dp->dev, _fmt)
#define dptx_info(_dp, _fmt...) dev_info(_dp->dev, _fmt)
#define dptx_warn(_dp, _fmt...) dev_warn(_dp->dev, _fmt)
#define dptx_err(_dp, _fmt...) dev_err(_dp->dev, _fmt)

#ifdef DPTX_DEBUG_AUX
#define dptx_dbg_aux(_dp, _fmt...) dev_dbg(_dp->dev, _fmt)
#else
#define dptx_dbg_aux(_dp, _fmt...)
#endif

#ifdef DPTX_DEBUG_IRQ
#define dptx_dbg_irq(_dp, _fmt...) dev_dbg(_dp->dev, _fmt)
#else
#define dptx_dbg_irq(_dp, _fmt...)
#endif

#endif
