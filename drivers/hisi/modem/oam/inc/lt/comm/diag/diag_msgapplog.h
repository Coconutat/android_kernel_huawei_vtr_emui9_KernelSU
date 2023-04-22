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


#ifndef __DIAG_MSGAPPLOG_H__
#define __DIAG_MSGAPPLOG_H__

#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif
#endif


/*****************************************************************************
  1 Include Headfile
*****************************************************************************/
#include "vos.h"
#include "mdrv.h"
#include "NVIM_Interface.h"
#include "msp_diag_comm.h"
#include "diag_common.h"
#include "msp_errno.h"


typedef VOS_UINT32 (*DIAG_APPLOG_PROC)(VOS_UINT8 *pData);

typedef struct
{
    DIAG_APPLOG_PROC   pFunc;
    VOS_UINT32         ulCmdId;
    VOS_UINT32         ulReserve;
}DIAG_APPLOG_PROC_FUN_STRU;

typedef enum _APPLOG_SWITCH_CFG
{
    APPLOG_SWITCHOFF,
    APPLOG_SWITCHON    
}APPLOG_SWITCH_CFG;

//typedef VOS_UINT32 APPLOG_SWITCH_CFG_UINT32;

typedef enum _APPLOG_LEVEL_CFG
{
    APPLOG_LEVEL_ERROR  =1,
    APPLOG_LEVEL_WARNING,
    APPLOG_LEVEL_NORMAL,
    APPLOG_LEVEL_INFO    
}APPLOG_LEVEL_CFG;

//typedef VOS_UINT32 APPLOG_LEVEL_CFG_UINT32;

typedef struct
{
    APPLOG_SWITCH_CFG      enable;
    APPLOG_LEVEL_CFG       level;
    VOS_UCHAR              resv[8];
}DIAG_APPLOG_CFG_REQ;


typedef struct
{
    VOS_UINT32 ulAuid;
    VOS_UINT32 ulSn;
    VOS_UINT32 ulRc;
}DIAG_APPLOG_CFG_CNF;
#define  APPLOG_DATA_OFFSET  8
VOS_VOID diag_AppLogMsgInit(VOS_VOID);
VOS_UINT32  diag_AppLogDataConfig(VOS_UINT8* pstReq);

#ifdef __cplusplus
    #if __cplusplus
        }
    #endif
#endif

#endif /* end of msp_diag.h */


