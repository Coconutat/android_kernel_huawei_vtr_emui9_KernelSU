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

#ifndef __BSP_DDR_H__
#define __BSP_DDR_H__

#ifdef __cplusplus
extern "C" {
#endif

#include "product_config.h"


/* FAMA macro -- start */

#if ((defined DDR_MEM_ADDR_FAMA) && (DDR_MEM_ADDR_FAMA != DDR_MEM_ADDR))

#define FAMA_DIFF  (DDR_MEM_ADDR_FAMA - DDR_MEM_ADDR)
#define MDDR_FAMA_TO(x)     (FAMA_DIFF+(unsigned long long)(x))
#define MDDR_FAMA_FROM(x)   (unsigned long)((unsigned long long)(x)-FAMA_DIFF)
#if defined(__KERNEL__) || defined(FASTBOOT_VERSION)
    #define MDDR_FAMA(x)    MDDR_FAMA_TO(x)
#else 
    #define MDDR_FAMA(x)    (x)
#endif
/* ipf,psam,cipher,socp 高32bit配置，传入值x为32bit数 */
/* 寄存器可配7bit，实际高3bit默认为0 128byte交织不动，只需要配置4bit */
#define MDDR_FAMA_CHAN(x)   ((unsigned long)(MDDR_FAMA_TO(x)>>32) & 0x7F)


#else

#define MDDR_FAMA_TO(x)     (x)
#define MDDR_FAMA_FROM(x)   (x)
#define MDDR_FAMA(x)        (x)
#define MDDR_FAMA_CHAN(x)   (0)

#endif

/* boston socp ddr fama映射方式不再与其他modem ddr保持一致。
   新增CFG_MODEM_SOCP_DDR_BASE.  开/关fama ddr值相同，高位为0  */
#if defined(MODEM_SOCP_DDR_BASE)
#define SOCP_MDDR_FAMA_TO(x)   (x)
#define SOCP_MDDR_FAMA_FROM(x) (x)
#define SOCP_MDDR_FAMA(x)      (x)
#define SOCP_MDDR_FAMA_CHAN(x) (0)
#else
#define SOCP_MDDR_FAMA_TO(x)   MDDR_FAMA_TO(x)
#define SOCP_MDDR_FAMA_FROM(x) MDDR_FAMA_FROM(x)
#define SOCP_MDDR_FAMA(x)      MDDR_FAMA(x)
#define SOCP_MDDR_FAMA_CHAN(x) MDDR_FAMA_CHAN(x)
#endif


/* FAMA  -- end */


/* M533 lpmcu ddr window macro -- start */
#ifdef LPMCU_DRAM_WINDOW

/* LPMCU memory map ddr的地址空间为 0x1000_0000~0x1FFF_FFFF,通过划窗映射到 ddr的实际地址空间。
   划窗默认配置为0，m3 访问地址 (0x1000_0000~0x1FFF_FFFF) 对应ddr实际地址空间 (0~0x0FFF_FFFF) */

/* 一般情况:已知ddr实际地址 获得m3 上的访问地址 */
#define LPMCU_DDR_PHY_TO_VIRT(phy_addr)       (((phy_addr) & 0x0FFFFFFFU) | 0x10000000U)
#define LPMCU_DDR_VIRT_TO_PHY(virt_addr)      (((virt_addr) & 0x0FFFFFFFU))


/* 暂不提供 VIRT_TO_PHY 以及 划窗配置 */
#if 0
/* 特殊情况需要改变划窗配置访问ddr */

#define DDR_ACCESS_WIN_REG              (0x4023F000U)  /* reg  */
#define DDR_ACCESS_WIN_MASK             (0xF0000000U)  /* win mask */

#define GET_DDR_ACCESS_WIN              (* ((u32 volatile *) (DDR_ACCESS_WIN_REG)))
#define SET_DDR_ACCESS_WIN(win_num)     (* ((u32 volatile *) (DDR_ACCESS_WIN_REG)) = ((win_num)<<28))

#define DDR_VIRT_TO_PHY(virt_addr)      (((virt_addr) & 0x0FFFFFFFU) | (GET_DDR_ACCESS_WIN&DDR_ACCESS_WIN_MASK))

#define DDR_PHY_TO_ACCESS_WIN_NUM(phy_addr)  (((phy_addr) & DDR_ACCESS_WIN_MASK)>>28)

#endif

#endif
/* M533 lpm3 ddr window macro  -- end */

#ifdef __cplusplus
}
#endif

#endif    /* __BSP_DDR_H__ */

