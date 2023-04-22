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

#ifndef __AT_LTE_COMMON_H__
#define __AT_LTE_COMMON_H__

#include "gen_msg.h"
#include "AtParse.h"

#include "AppEsmInterface.h"
#include "AppMmInterface.h"
#include "AppRrcInterface.h"
#include "ATCmdProc.h"


#ifdef __cplusplus
extern "C"
{
#endif

#define AT_LTE_EVENT_NULL_SIZE              0

#define AT_RSSI_LOW                             (-113)
#define AT_RSSI_HIGH                            (-51)
#define AT_CSQ_RSSI_LOW                         (0)
#define AT_CSQ_RSSI_HIGH                        (31)
#define AT_RSSI_UNKNOWN                         (99)

#define AT_BER_UNKNOWN                          (99)

enum
{
    EN_SERVICE_CELL_ID,
    EN_SYN_FREQ_CELL_ID,
    EN_ASYN_FREQ_CELL_ID,
    EN_ASYN_UMTS_CELL_ID,
    EN_ASYN_GSM_CELL_ID,
    EN_CELL_ID_BUTT
};

typedef enum
{
    AT_CMD_LTE_BEGIN = AT_CMD_COMM_BUTT,
    AT_CMD_LWCLASH,
    AT_CMD_RADVER,
    AT_CMD_LCELLINFO,
    AT_CMD_LTCOMMCMD,
    AT_CMD_LCACELL,
    AT_CMD_LTE_BUTT
}AT_LTE_CMD_INDEX_ENUM;

typedef enum
{
    AT_CMD_LTE_CURR_BEGIN = AT_CMD_COMM_CURRENT_OPT,
    AT_CMD_LWCLASH_READ,
    AT_CMD_RADVER_SET,
    AT_CMD_LCELLINFO_QUERY,
    AT_CMD_CERSSI_QUERY,
    AT_CMD_PSTANDBY_SET,
    AT_CMD_LTCOMMCMD_SET,
    AT_CMD_LTCOMMCMD_READ,
    AT_CMD_FPLLSTATUS_READ,
    AT_CMD_LCACELL_QUERY,
    AT_CMD_LTE_CURR_BUTT
}AT_LTE_CMD_CURRENT_OPT_ENUM;

typedef struct
{
    VOS_UINT32 ulSrcError;   /* USIM,NAS...
 */
    VOS_UINT32 ulATError;    /* AT错误码
 */
}AT_ERROR_CODE_TABLE_STRU;

/*lint -e958 -e959 修改人:l60609;原因:64bit*/
typedef struct{
    VOS_UINT32 ulMsgId;
    PFN_AT_FW_MSG_PROC   pfnCnfMsgProc;
}AT_FTM_CNF_MSG_PROC_STRU;
/*lint +e958 +e959 修改人:l60609;原因:64bit*/


/*+CSQ
 */
typedef struct
{
    VOS_UINT8 ucRssiValue;
    VOS_UINT8 ucChannalQual;
}TAF_RSSI_STRU;

extern AT_SEND_DATA_BUFFER_STRU gstLAtSendData;
extern VOS_UINT8 *pgucLAtSndCodeAddr;

VOS_UINT32 initParaListS16( AT_PARSE_PARA_TYPE_STRU *pPara, VOS_UINT16 ulListLen, VOS_INT16* pausList);
VOS_UINT32 initParaListU16( AT_PARSE_PARA_TYPE_STRU *pPara, VOS_UINT16 ulListLen, VOS_UINT16* pausList);


extern VOS_VOID CmdErrProc(VOS_UINT8 ucClientId, VOS_UINT32 ulErrCode, VOS_UINT16 usBufLen, VOS_UINT8* pucBuf);

extern VOS_UINT32 atSetSdloadCnf(VOS_UINT8 ucClientId,VOS_VOID *pMsgBlock);


/* AT模块给FTM 模块发送消息
 */
extern VOS_UINT32 atSendFtmDataMsg(VOS_UINT32 TaskId, VOS_UINT32 MsgId, VOS_UINT32 ulClientId, VOS_VOID* pData, VOS_UINT32 uLen);

extern VOS_UINT32 atSendL4aDataMsg(MN_CLIENT_ID_T usClientId, VOS_UINT32 TaskId, VOS_UINT32 MsgId, VOS_VOID* pData, VOS_UINT32 uLen);

extern VOS_UINT32 atSetFWAVEParaCnfProc(VOS_UINT8 ucClientId, VOS_VOID * pMsgBlock);



extern VOS_UINT32 At_SetLCellInfoPara(VOS_UINT8 ucIndex);

extern VOS_UINT32 AT_SetLFastDormPara(VOS_UINT8 ucIndex);
extern VOS_UINT32 AT_SetLIndCfgReq(VOS_UINT8 ucIndex,L4A_IND_CFG_STRU* pstIndCfgReq);
extern VOS_UINT32 AtQryLCerssiPara(VOS_UINT8 ucIndex);
extern VOS_UINT32 At_SetLFromConnToIdlePara(VOS_UINT8 ucIndex);
extern VOS_UINT32 AT_QryLwclashPara(VOS_UINT8 ucIndex);

extern VOS_UINT32 AT_QryLcacellPara(VOS_UINT8 ucIndex);

extern VOS_UINT32 atSetNVFactoryBack(VOS_UINT8 ucClientId);
extern VOS_UINT32 atSetNVFactoryRestore(VOS_UINT8 ucClientId);

extern VOS_UINT32 atSetLTCommCmdPara(VOS_UINT8 ucClientId);
extern VOS_UINT32 atSetLTCommCmdParaCnfProc(VOS_UINT8 ucClientId, VOS_VOID * pMsgBlock);
extern VOS_UINT32 atQryLTCommCmdPara(VOS_UINT8 ucClientId);
extern VOS_UINT32 atQryLTCommCmdParaCnfProc(VOS_UINT8 ucClientId, VOS_VOID * pMsgBlock);


extern VOS_UINT32 At_RegisterTLCmdTable(VOS_VOID);


extern VOS_UINT32 AT_SetRadverPara(VOS_UINT8 ucIndex);
extern VOS_UINT32 atSetRadverCnfProc(VOS_VOID *pMsgBlock);



extern VOS_UINT32 atQryFPllStatusPara(VOS_UINT8 ucClientId);
extern VOS_UINT32 atQryFPllStatusParaCnfProc(VOS_UINT8 ucClientId, VOS_VOID *pMsgBlock);

#ifdef __cplusplus
}
#endif


#endif /*__AT_H__
 */



