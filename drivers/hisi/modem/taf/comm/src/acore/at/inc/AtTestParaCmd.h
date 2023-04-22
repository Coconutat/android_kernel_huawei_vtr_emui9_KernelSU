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

#ifndef __ATTESTPARACMD_H__
#define __ATTESTPARACMD_H__

/*****************************************************************************
  1 其他头文件包含
*****************************************************************************/
#include "AtParse.h"
#include "product_config.h"

#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif
#endif


#pragma pack(4)

/*****************************************************************************
  2 宏定义
*****************************************************************************/

/*****************************************************************************
  3 枚举定义
*****************************************************************************/


/*****************************************************************************
  4 全局变量声明
*****************************************************************************/


/*****************************************************************************
  5 消息头定义
*****************************************************************************/


/*****************************************************************************
  6 消息定义
*****************************************************************************/


/*****************************************************************************
  7 STRUCT定义
*****************************************************************************/


/*****************************************************************************
  8 UNION定义
*****************************************************************************/


/*****************************************************************************
  9 OTHERS定义
*****************************************************************************/


/*****************************************************************************
  10 函数声明
*****************************************************************************/
extern VOS_UINT32 At_CmdTestProcOK(VOS_UINT8 ucIndex);

extern VOS_UINT32 At_CmdTestProcERROR(VOS_UINT8 ucIndex);

extern VOS_UINT32 AT_TestRefclkfreqPara(VOS_UINT8 ucIndex);

extern VOS_UINT32 AT_TestNCellMonitorPara(VOS_UINT8 ucIndex);

extern VOS_UINT32 AT_TestCbgPara( VOS_UINT8 ucIndex );

extern VOS_UINT32 AT_TestCgpsClockPara(VOS_UINT8 ucIndex);

extern VOS_UINT32 At_TestCgdcont(VOS_UINT8 ucIndex);

extern VOS_UINT32 At_TestCgdscont(VOS_UINT8 ucIndex);

extern VOS_UINT32 At_TestCgtft(VOS_UINT8 ucIndex);

extern VOS_UINT32 At_TestCgact(VOS_UINT8 ucIndex);

extern VOS_UINT32 At_TestCgcmod(VOS_UINT8 ucIndex);

extern VOS_UINT32 At_TestCgpaddr(VOS_UINT8 ucIndex);

extern VOS_UINT32 At_TestCgautoPara(VOS_UINT8 ucIndex);

extern VOS_UINT32 At_TestAuhtdata(VOS_UINT8 ucIndex);

extern VOS_UINT32 At_TestNdisconn(VOS_UINT8 ucIndex);

extern VOS_UINT32 At_TestNdisDup(VOS_UINT8 ucIndex);

extern VOS_UINT32 At_TestCgeqos(VOS_UINT8 ucIndex);

extern VOS_UINT32 At_TestCgeqosrdp(VOS_UINT8 ucIndex);

extern VOS_UINT32 At_TestCgcontrdp(VOS_UINT8 ucIndex);

extern VOS_UINT32 At_TestCgscontrdp(VOS_UINT8 ucIndex);

extern VOS_UINT32 At_TestCgtftrdp(VOS_UINT8 ucIndex);

extern VOS_UINT32 AT_TestChrgEnablePara(VOS_UINT8 ucIndex);

extern VOS_UINT32 AT_TestCmutPara(VOS_UINT8 ucIndex);

extern VOS_UINT32 At_TestVtsPara(VOS_UINT8 ucIndex);

extern VOS_UINT32 At_TestCuus1Para(VOS_UINT8 ucIndex);

extern VOS_UINT32 At_TestCcwaPara(VOS_UINT8 ucIndex);

extern VOS_UINT32 At_TestCcfcPara(VOS_UINT8 ucIndex);

extern VOS_UINT32 At_TestCusdPara(VOS_UINT8 ucIndex);

extern VOS_UINT32 At_TestCpwdPara(VOS_UINT8 ucIndex);

extern VOS_UINT32 At_TestClckPara(VOS_UINT8 ucIndex);

extern VOS_UINT32 AT_TestCpbsPara(VOS_UINT8 ucIndex);

extern VOS_UINT32 AT_TestCpbrPara(VOS_UINT8 ucIndex);

extern VOS_UINT32 AT_TestCnmaPara(VOS_UINT8 ucIndex);

extern VOS_UINT32 AT_TestCpasPara(VOS_UINT8 ucIndex);

#if ((FEATURE_ON == FEATURE_GCBS) || (FEATURE_ON == FEATURE_WCBS))
extern VOS_UINT32 AT_TestCscbPara(VOS_UINT8 ucIndex);
#endif

/* Added by f62575 for AT Project, 2011-10-28, begin */
extern VOS_UINT32 AT_TestCpmsPara(VOS_UINT8 ucIndex);
/* Added by f62575 for AT Project, 2011-10-28, end */

extern VOS_UINT32 At_TestCopsPara(TAF_UINT8 ucIndex);

extern VOS_UINT32 At_TestCpolPara(TAF_UINT8 ucIndex);

extern VOS_UINT32 At_TestCgeqnegPara(VOS_UINT8 ucIndex);

#if (FEATURE_ON == FEATURE_CSG)
extern VOS_UINT32 AT_TestCsgIdSearchPara(VOS_UINT8 ucIndex);
#endif

extern VOS_UINT32 At_TestCgeqreqPara(VOS_UINT8 ucIndex);

extern VOS_UINT32 At_TestCgansPara(VOS_UINT8 ucIndex);

extern VOS_UINT32 At_TestCgansExtPara(VOS_UINT8 ucIndex);

extern VOS_UINT32 At_TestCgdnsPara(VOS_UINT8 ucIndex);

extern VOS_UINT32 At_TestCgdataPara(VOS_UINT8 ucIndex);

#if (FEATURE_ON == FEATURE_IMS)
extern VOS_UINT32 AT_TestCiregPara(VOS_UINT8 ucIndex);

extern VOS_UINT32 AT_TestCirepPara(VOS_UINT8 ucIndex);
#endif

#if (FEATURE_ON == FEATURE_AT_HSUART)
VOS_UINT32 AT_TestIprPara(VOS_UINT8 ucIndex);
VOS_UINT32 AT_TestIcfPara(VOS_UINT8 ucIndex);
VOS_UINT32 AT_TestIfcPara(VOS_UINT8 ucIndex);
#endif





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

#endif /* end of AtCmdCallProc.h */

