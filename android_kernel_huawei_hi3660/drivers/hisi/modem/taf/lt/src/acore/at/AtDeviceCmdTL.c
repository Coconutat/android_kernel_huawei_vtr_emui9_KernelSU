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

/*****************************************************************************
  1 头文件包含
*****************************************************************************/
/*lint --e{7,537,305,322}*/
#include "AtParse.h"
#include "ATCmdProc.h"
#include "AtDeviceCmd.h"
#include "AtCheckFunc.h"
#include "mdrv.h"
#include "AtCmdMsgProc.h"
#include "LNvCommon.h"
#include "at_lte_common.h"

/* 定义了LTE与TDS私有装备AT命令 */
AT_PAR_CMD_ELEMENT_STRU g_astAtDeviceCmdTLTbl[] = {
    /*BEGIN: LTE 快速校准装备AT命令 */
	{AT_CMD_NVRDLEN,
	 atSetNVRDLenPara,		 AT_SET_PARA_TIME,	VOS_NULL_PTR,		 AT_NOT_SET_TIME,  VOS_NULL_PTR, AT_NOT_SET_TIME,
	 VOS_NULL_PTR,		  AT_NOT_SET_TIME,
	 AT_ERROR, CMD_TBL_PIN_IS_LOCKED,
	 (VOS_UINT8*)"^NVRDLEN",(VOS_UINT8*)"(0-65535)"},

	 {AT_CMD_NVRDEX,
	 atSetNVRDExPara,		AT_SET_PARA_TIME,  VOS_NULL_PTR,		AT_NOT_SET_TIME,  VOS_NULL_PTR, AT_NOT_SET_TIME,
	 VOS_NULL_PTR,		  AT_NOT_SET_TIME,
	 AT_ERROR, CMD_TBL_PIN_IS_LOCKED,
	 (VOS_UINT8*)"^NVRDEX",(VOS_UINT8*)"(0-65535),(0-2048),(0-2048)"},

    {AT_CMD_NVWREX,
    atSetNVWRExPara,       AT_SET_PARA_TIME,  VOS_NULL_PTR,        AT_NOT_SET_TIME,  VOS_NULL_PTR, AT_NOT_SET_TIME,
    VOS_NULL_PTR,        AT_NOT_SET_TIME,
    AT_ERROR, CMD_TBL_PIN_IS_LOCKED,
    (VOS_UINT8*)"^NVWREX",(VOS_UINT8*)"(0-65535),(0-2048),(0-2048),(@data),(@data),(@data),(@data),(@data),(@data),(@data),(@data),(@data),(@data),(@data),(@data)"},
    /*END: LTE 快速校准装备AT命令 */


    {AT_CMD_LTCOMMCMD,
    atSetLTCommCmdPara,     AT_SET_PARA_TIME,     atQryLTCommCmdPara,     AT_QRY_PARA_TIME, VOS_NULL_PTR, AT_NOT_SET_TIME,
    VOS_NULL_PTR,        AT_NOT_SET_TIME,
    AT_ERROR, CMD_TBL_PIN_IS_LOCKED,
    (VOS_UINT8*)"^LTCOMMCMD",  (VOS_UINT8*)"(0-65535),(0-2000),(0-65535),(0-65535),(@data),(@data),(@data),(@data)"},    

};

/*****************************************************************************
 函 数 名  : At_RegisterDeviceCmdTLTable
 功能描述  : 注册装备命令表
 输入参数  : VOS_VOID
 输出参数  : 无
 返 回 值  : VOS_UINT32
 调用函数  :
 被调函数  :
*****************************************************************************/
VOS_UINT32 At_RegisterDeviceCmdTLTable(VOS_VOID)
{
    return AT_RegisterCmdTable(g_astAtDeviceCmdTLTbl, sizeof(g_astAtDeviceCmdTLTbl)/sizeof(g_astAtDeviceCmdTLTbl[0]));
}


