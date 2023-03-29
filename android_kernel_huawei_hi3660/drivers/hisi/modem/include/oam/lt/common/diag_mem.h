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

#ifndef __DIAG_MEM_H__
#define __DIAG_MEM_H__

#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif
#endif


/*****************************************************************************
  1 Include Headfile
*****************************************************************************/
#include "product_config.h"
#include "vos.h"



#pragma pack(4)

/*****************************************************************************
  2 Macro
*****************************************************************************/



/*diag mem*/

#ifdef ENABLE_DIAG_FIX_ADDR

#define DIAG_CODER_SRC_ACORE_CNF_PADDR      (DDR_SOCP_ADDR)
#define DIAG_CODER_SRC_ACORE_CNF_LENGTH     (1024*64)

#define DIAG_CODER_SRC_ACORE_IND_PADDR      (DIAG_CODER_SRC_ACORE_CNF_PADDR+DIAG_CODER_SRC_ACORE_CNF_LENGTH)
#define DIAG_CODER_SRC_ACORE_IND_LENGTH     (1024*1024)

#define DIAG_CODER_SRC_CCORE_CNF_PADDR      (DIAG_CODER_SRC_ACORE_IND_PADDR+DIAG_CODER_SRC_ACORE_IND_LENGTH)
#define DIAG_CODER_SRC_CCORE_CNF_LENGTH     (1024*64)

#define DIAG_CODER_SRC_CCORE_IND_PADDR      (DIAG_CODER_SRC_CCORE_CNF_PADDR+DIAG_CODER_SRC_CCORE_CNF_LENGTH)
#define DIAG_CODER_SRC_CCORE_IND_LENGTH     (1536*1024)


#define DIAG_CORER_SRC_DSP_PADDR            (DIAG_CODER_SRC_CCORE_IND_PADDR+DIAG_CODER_SRC_CCORE_IND_LENGTH)
#define DIAG_CORER_SRC_DSP_LENGTH           (1024*1024)

#define BBP_LOG0_MEM_ADDR                   (DIAG_CORER_SRC_DSP_PADDR+DIAG_CORER_SRC_DSP_LENGTH)
#define BBP_LOG0_MEM_SIZE                   (64*1024)

#define BBP_LOG1_MEM_ADDR                   (BBP_LOG0_MEM_ADDR + BBP_LOG0_MEM_SIZE)
#define BBP_LOG1_MEM_SIZE                   (8*1024)

#define SOCP_CODER_SRC_GUDSP_ADDR            (BBP_LOG1_MEM_ADDR+BBP_LOG1_MEM_SIZE)
#define SOCP_CODER_SRC_GUDSP_LENGTH          (128*1024)

#define SOCP_CODER_SRC_XDSP_ADDR             (SOCP_CODER_SRC_GUDSP_ADDR+SOCP_CODER_SRC_GUDSP_LENGTH)
#define SOCP_CODER_SRC_XDSP_LENGTH           (512*1024)

#define SOCP_CODER_SRC_HIFI_ADDR             (SOCP_CODER_SRC_XDSP_ADDR+SOCP_CODER_SRC_XDSP_LENGTH)
#define SOCP_CODER_SRC_HIFI_LENGTH           (32*1024)


#define BBP_SOCP_ADDR                       (DDR_SOCP_ADDR+8*1024*1024)
#define BBP_SOCP_SIZE                       (DDR_SOCP_SIZE-8*1024*1024)


#if (VOS_LINUX == VOS_OS_VER)

#define DIAG_CODER_SRC_CNF_PADDR       (DIAG_CODER_SRC_ACORE_CNF_PADDR)
#define DIAG_CODER_SRC_CNF_LENGTH      (DIAG_CODER_SRC_ACORE_CNF_LENGTH)

#define DIAG_CODER_SRC_IND_PADDR       (DIAG_CODER_SRC_ACORE_IND_PADDR)
#define DIAG_CODER_SRC_IND_LENGTH      (DIAG_CODER_SRC_ACORE_IND_LENGTH)

#else  /*ACORE*/

#define DIAG_CODER_SRC_CNF_PADDR       (DIAG_CODER_SRC_CCORE_CNF_PADDR)
#define DIAG_CODER_SRC_CNF_LENGTH      (DIAG_CODER_SRC_CCORE_CNF_LENGTH)

#define DIAG_CODER_SRC_IND_PADDR        (DIAG_CODER_SRC_CCORE_IND_PADDR)
#define DIAG_CODER_SRC_IND_LENGTH       (DIAG_CODER_SRC_CCORE_IND_LENGTH)

#endif /*CCORE*/

#else

#define BBP_SOCP_ADDR                   (DDR_SOCP_ADDR)

#define BBP_LOG0_MEM_SIZE                   (64*1024)
#define BBP_LOG1_MEM_SIZE                   (8*1024)

#endif



/********************* BBP DSÄÚ´æÅäÖÃ **************************
* BBP LOG 0:  64K
* BBP LOG 1:   8K
* BBP DS   :  Ëæ»ú
* TOTAL SIZE: BBPDS + 120K
****************************************************************/


#define BBP_DS_MEM_ADDR                         	(BBP_SOCP_ADDR)
#define BBP_DS_MEM_SIZE                             (BBP_SOCP_SIZE)


/*****************************************************************************
  3 Massage Declare
*****************************************************************************/

/*****************************************************************************
  4 Enum
*****************************************************************************/


/*****************************************************************************
  5 STRUCT
*****************************************************************************/


/*****************************************************************************
  6 UNION
*****************************************************************************/


/*****************************************************************************
  7 Extern Global Variable
*****************************************************************************/


/*****************************************************************************
  8 Fuction Extern
*****************************************************************************/


/*****************************************************************************
  9 OTHERS
*****************************************************************************/



#if (VOS_OS_VER == VOS_WIN32)
#pragma pack()
#else
#pragma pack(0)
#endif




#ifdef __cplusplus
    #if __cplusplus
        }
    #endif
#endif

#endif /* end of diag_mem.h */
