/*
 * Copyright (C) Huawei Technologies Co., Ltd. 2012-2015. All rights reserved.
 * foss@huawei.com
 *
 * If distributed as part of the Linux kernel, the following license terms
 * apply:
 *
 * * This program is free software; you can redistribute it and/or modify
 * * it under the terms of the GNU General Public License version 2 and
 * * only version 2 as published by the Free Software Foundation.
 * *
 * * This program is distributed in the hope that it will be useful,
 * * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * * GNU General Public License for more details.
 * *
 * * You should have received a copy of the GNU General Public License
 * * along with this program; if not, write to the Free Software
 * * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307, USA
 *
 * Otherwise, the following license terms apply:
 *
 * * Redistribution and use in source and binary forms, with or without
 * * modification, are permitted provided that the following conditions
 * * are met:
 * * 1) Redistributions of source code must retain the above copyright
 * *    notice, this list of conditions and the following disclaimer.
 * * 2) Redistributions in binary form must reproduce the above copyright
 * *    notice, this list of conditions and the following disclaimer in the
 * *    documentation and/or other materials provided with the distribution.
 * * 3) Neither the name of Huawei nor the names of its contributors may
 * *    be used to endorse or promote products derived from this software
 * *    without specific prior written permission.
 *
 * * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 *
 */

#ifndef __BSP_DSPLOAD_H__
#define __BSP_DSPLOAD_H__

#ifdef __cplusplus
extern "C" {
#endif

#include <bsp_shared_ddr.h>
#include <of.h>
#if defined(__OS_VXWORKS__) || defined(__OS_RTOSCK__) || defined(__OS_RTOSCK_SMP__)
#include <bsp_dpm.h>
#include <mdrv_dsp.h>
#include <bsp_edma.h>
#endif

struct bbe_dump_proc_flag
{
    unsigned int dsp_dump_flag;         /* 异常流程中，记录dsp保存tcm的过程标志 */
};

struct dsp_img_head_info
{
    u8 dsp_img_type :4;                 /*dsp镜像类型*/
    u8 reserved :4;
};

struct dsp_sect_desc_stru
{
    u8                          usNo;                   /* 段序号 */
    struct dsp_img_head_info    ucImgHeadInfo;
    u8                          ucTcmType;              /* 段类型: 0-代码(text); 1-rodata; 2-data; 3-bss */
    u8                          ucLoadType;             /* 加载类型: 0-每次上电时都加载; 1-上电时只加载一次; 2-不需要底软加载 */
    u32                         ulFileOffset;           /* 段在文件内的偏移 */
    u32                         ulTargetAddr;           /* 加载的目标地址 */
    u32                         ulSectSize;             /* 段的大小 */
};

/* TLBBE16/CBBE16镜像头 */
struct dsp_bin_header_stru
{
    s8                          acDescription[24];      /* 由工具生成，内容为处理器标记和日期、时间 */
    u32                         ulFileSize;             /* 文件大小 */
    u32                         ulSectNum;              /* 段个数 */
    struct dsp_sect_desc_stru   astSect[0];             /* 段信息 */
};

#ifndef CONFIG_CBBE

#ifdef __KERNEL__
static inline int his_load_cdsp_image(void)
{
    return 0;
}
#endif

#if defined(__OS_VXWORKS__) || defined(__OS_RTOSCK__) || defined(__OS_RTOSCK_SMP__)
static inline void dsp_load_init(void)
{
    return;
}

static inline int bsp_bbe_load(enum bsp_dsp_type_e etype)
{
    return 0;
}

static inline int bsp_bbe_store_ex(enum bsp_dsp_type_e etype)
{
    return 0;
}

static inline int bsp_bbe_wait_store_over(enum bsp_dsp_type_e etype)
{
    return 0;
}

static inline int bsp_bbe_restore_ex(enum bsp_dsp_type_e etype)
{
    return 0;
}

static inline int bsp_bbe_wait_restore_over(enum bsp_dsp_type_e etype)
{
    return 0;
}

static inline int bsp_bbe_power_on_ex(enum bsp_dsp_type_e etype)
{
    return 0;
}

static inline int bsp_bbe_power_off_ex(enum bsp_dsp_type_e etype)
{
    return 0;
}

static inline int bsp_bbe_run_ex(enum bsp_dsp_type_e etype)
{
    return 0;
}

static inline int bsp_bbe_stop_ex(enum bsp_dsp_type_e etype)
{
    return 0;
}

static inline int bsp_bbe_clk_enable(enum bsp_dsp_type_e etype)
{
    return 0;
}

static inline int bsp_bbe_clk_disable(enum bsp_dsp_type_e etype)
{
    return 0;
}

static inline int bsp_bbe_unreset_ex(enum bsp_dsp_type_e etype)
{
    return 0;
}

static inline int bsp_bbe_reset_ex(enum bsp_dsp_type_e etype)
{
    return 0;
}

static inline int bsp_dsp_clk_enable(enum bsp_dsp_type_e etype)
{
    return 0;
}

static inline int bsp_dsp_clk_disable(enum bsp_dsp_type_e etype)
{
    return 0;
}

static inline int bsp_dsp_unreset_ex(enum bsp_dsp_type_e etype)
{
    return 0;
}

static inline int bsp_dsp_reset_ex(enum bsp_dsp_type_e etype)
{
    return 0;
}

static inline int dsp_parse_image_header(enum bsp_dsp_type_e etype)
{
    return 0;
}

static inline void dsp_load_once_sect(enum bsp_dsp_type_e etype)
{
    return;
}

static inline int dsp_edma_lli_start(enum bsp_dsp_type_e etype, struct edma_cb *pedmacb)
{
    return 0;
}

static inline int dsp_restore_edma_lli_init(enum bsp_dsp_type_e etype)
{
    return 0;
}

static inline int dsp_store_edma_lli_init(enum bsp_dsp_type_e etype)
{
    return 0;
}

static inline int bsp_dsp_dfs_pll_enable_ex(enum bsp_dsp_type_e etype)
{
    return 0;
}

static inline int bsp_dsp_dfs_pll_disable_ex(enum bsp_dsp_type_e etype)
{
    return 0;
}

static inline int bsp_bbe_power_up_ex(enum bsp_dsp_type_e etype)
{
    return 0;
}

static inline int bsp_bbe_power_down_ex(enum bsp_dsp_type_e etype)
{
    return 0;
}

static inline int bsp_dsp_store_dual_data_force(enum bsp_dsp_type_e etype)
{
    return 0;
}

static inline int bsp_dsp_store_tcm_force(enum bsp_dsp_type_e etype)
{
    return 0;
}


static inline int bsp_dsp_store_dtcm_normal(enum bsp_dsp_type_e etype)
{
    return 0;
}

static inline int bsp_dsp_restore_tcm_normal(enum bsp_dsp_type_e etype)
{
    return 0;
}

static inline int bsp_dsp_wait_store_over_normal(enum bsp_dsp_type_e etype)
{
    return 0;
}

static inline int bsp_dsp_wait_restore_over_normal(enum bsp_dsp_type_e etype)
{
    return 0;
}

static inline int bsp_dsp_pll_status_ex(enum bsp_dsp_type_e etype)
{
    return 0;
}

static inline int bsp_dsp_dfs_pll_status_ex(enum bsp_dsp_type_e etype)
{
    return 0;
}

static inline int bsp_dsp_get_addr_info(enum bsp_dsp_type_e etype, BSP_DSP_ADDR_INFO_STRU *pAddrInfo)
{
    return 0;
}

static inline void dsp_load_help(enum bsp_dsp_type_e etype)
{
    return;
}

static inline void bsp_bbe_refclk_enable_ex(enum bsp_dsp_type_e etype)
{
    return;
}

static inline void bsp_bbe_refclk_disable_ex(enum bsp_dsp_type_e etype)
{
    return;
}

static inline void bsp_bbe_peri_refclk_enable_ex(enum bsp_dsp_type_e etype)
{
    return;
}

static inline void bsp_bbe_peri_refclk_disable_ex(enum bsp_dsp_type_e etype)
{
    return;
}

static inline void bsp_dsp_om_log_ex(enum bsp_dsp_type_e etype)
{
    return;
}

static inline int bsp_dsp_load_init(struct device_node * dsp_device)
{
    return 0;
}

static inline int bsp_dsp_pwrctrl_init(struct device_node * dsp_device)
{
    return 0;
}

static inline int bsp_dsp_pllctrl_init(struct device_node * dsp_device)
{
    return 0;
}

static inline int bsp_dsp_debug_init(struct device_node * dsp_device)
{
    return 0;
}

#endif

#else
#ifdef __KERNEL__
int his_load_cdsp_image(void);
#endif

#if defined(__OS_VXWORKS__) || defined(__OS_RTOSCK__) || defined(__OS_RTOSCK_SMP__)
int bsp_bbe_load(enum bsp_dsp_type_e etype);
int bsp_bbe_store_ex(enum bsp_dsp_type_e etype);
int bsp_bbe_wait_store_over(enum bsp_dsp_type_e etype);
int bsp_bbe_restore_ex(enum bsp_dsp_type_e etype);
int bsp_bbe_wait_restore_over(enum bsp_dsp_type_e etype);
int bsp_bbe_power_on_ex(enum bsp_dsp_type_e etype);
int bsp_bbe_power_off_ex(enum bsp_dsp_type_e etype);
int bsp_bbe_run_ex(enum bsp_dsp_type_e etype);
int bsp_bbe_stop_ex(enum bsp_dsp_type_e etype);
int bsp_bbe_clk_enable(enum bsp_dsp_type_e etype);
int bsp_bbe_clk_disable(enum bsp_dsp_type_e etype);
int bsp_bbe_unreset_ex(enum bsp_dsp_type_e etype);
int bsp_bbe_reset_ex(enum bsp_dsp_type_e etype);
int bsp_dsp_clk_enable(enum bsp_dsp_type_e etype);
int bsp_dsp_clk_disable(enum bsp_dsp_type_e etype);
int bsp_dsp_unreset_ex(enum bsp_dsp_type_e etype);
int bsp_dsp_reset_ex(enum bsp_dsp_type_e etype);
int dsp_parse_image_header(enum bsp_dsp_type_e etype);
void dsp_load_once_sect(enum bsp_dsp_type_e etype);
int dsp_edma_lli_start(enum bsp_dsp_type_e etype, struct edma_cb *pedmacb);
int dsp_restore_edma_lli_init(enum bsp_dsp_type_e etype);
int dsp_store_edma_lli_init(enum bsp_dsp_type_e etype);
int bsp_dsp_dfs_pll_enable_ex(enum bsp_dsp_type_e etype);
int bsp_dsp_dfs_pll_disable_ex(enum bsp_dsp_type_e etype);
int bsp_bbe_power_up_ex(enum bsp_dsp_type_e etype);
int bsp_bbe_power_down_ex(enum bsp_dsp_type_e etype);
int bsp_dsp_store_dual_data_force(enum bsp_dsp_type_e etype);
int bsp_dsp_store_tcm_force(enum bsp_dsp_type_e etype);
int bsp_dsp_store_dtcm_normal(enum bsp_dsp_type_e etype);
int bsp_dsp_restore_tcm_normal(enum bsp_dsp_type_e etype);
int bsp_dsp_wait_store_over_normal(enum bsp_dsp_type_e etype);
int bsp_dsp_wait_restore_over_normal(enum bsp_dsp_type_e etype);
int bsp_dsp_pll_status_ex(enum bsp_dsp_type_e etype);
int bsp_dsp_dfs_pll_status_ex(enum bsp_dsp_type_e etype);
int bsp_dsp_get_addr_info(enum bsp_dsp_type_e etype, BSP_DSP_ADDR_INFO_STRU *pAddrInfo);
void bsp_dsp_om_log_ex(enum bsp_dsp_type_e etype);
void bsp_bbe_refclk_enable_ex(enum bsp_dsp_type_e etype);
void bsp_bbe_refclk_disable_ex(enum bsp_dsp_type_e etype);
void bsp_bbe_peri_refclk_enable_ex(enum bsp_dsp_type_e etype);
void bsp_bbe_peri_refclk_disable_ex(enum bsp_dsp_type_e etype);
void dsp_load_help(enum bsp_dsp_type_e etype);
int bsp_dsp_load_init(struct device_node * dsp_device);
int bsp_dsp_pwrctrl_init(struct device_node * dsp_device);
int bsp_dsp_pllctrl_init(struct device_node * dsp_device);
int bsp_dsp_debug_init(struct device_node * dsp_device);
int bsp_dsp_init_reg(BSP_DSP_TYPE_E etype, BSP_DSP_INIT_CB_FUNC pfun, char* param);
#endif
#endif

#ifdef __cplusplus
}
#endif

#endif

